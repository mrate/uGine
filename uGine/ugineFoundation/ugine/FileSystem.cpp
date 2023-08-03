#include "FileSystem.h"

#include <ugine/Locking.h>
#include <ugine/Profile.h>
#include <ugine/Scheduler.h>
#include <ugine/Thread.h>

#include <fstream>

#if defined(_WIN32)
#include <Windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <winerror.h>

#pragma comment(lib, "shlwapi.lib")
#endif

namespace ugine {

class FileSystemDir final : public FileSystem {
public:
    struct IoSchedulerTask : PinnedTask {
        IoSchedulerTask(u32 threadId, FileSystemDir& fs)
            : fs_{ fs } {
            threadNum = threadId;
        }

        void Execute() { fs_.IoThread(); }

        FileSystemDir& fs_;
    };

    FileSystemDir(const Path& root, IAllocator& allocator)
        : allocator_{ allocator }
        , root_{ root } {

        Init();

        thread_ = Thread{
            "I/O thread",
            [this] {
                PROFILE_THREAD("IO Thread");

                IoThread();
            },
            Thread::Priority::Normal,
            allocator_,
        };
    }

    FileSystemDir(const Path& root, Scheduler& scheduler, u32 threadId, IAllocator& allocator)
        : allocator_{ allocator }
        , root_{ root }
        , scheduler_{ &scheduler } {
        Init();

        schedulerTask_ = MakeUnique<IoSchedulerTask>(allocator, threadId, *this);
        scheduler.SchedulePinned(schedulerTask_.Get());
    }

    ~FileSystemDir() {
        running_ = false;
        cv_.Notify();

        if (schedulerTask_) {
            scheduler_->WaitFor(schedulerTask_.Get());
        } else {
            UGINE_ASSERT(thread_.Joinable());

            thread_.Join();
        }
    }

    void Init() {
        PROFILE_PLOT_NUMBER("IO tasks", true, 0x00ff00ff);
        PROFILE_PLOT("IO tasks", i64(0));

        running_ = true;
    }

    bool ReadOnly() const override { return false; }

    bool Read(const Path& path, Vector<u8>& data) override {
        std::ifstream file{ (root_ / path).Data(), std::ios::binary };
        if (!file.good()) {
            return false;
        }

        file.seekg(0, std::ios::end);
        const auto size{ file.tellg() };
        file.seekg(0, std::ios::beg);

        data.Resize(size);
        file.read(reinterpret_cast<char*>(data.Data()), size);

        return true;
    }

    bool Write(const Path& path, Span<const u8> data) override {
        std::ofstream file{ (root_ / path).Data(), std::ios::binary };
        if (file.fail()) {
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.Data()), data.Size());
        file.close();
        return true;
    }

    void SyncPoint() override {
        PROFILE_EVENT_NC("IO sync", COLOR_PROFILE_FILESYSTEM);

        Vector<IoTask> ready{ allocator_ };

        {
            Lock lock{ mutex_ };
            ready.Swap(ready_);
        }

        for (auto& task : ready) {
            if (!task.flags.cancelled) {
                task.callback(task.data.ToSpan(), task.flags.success);
            }
        }
    }

    RequestHandle ReadAsync(const Path& path, Callback cb, bool anyThread) override {
        RequestHandle req{};

        {
            Lock lock{ mutex_ };
            req = ++counter_ == 0 ? ++counter_ : counter_;
            pending_.PushBack(IoTask{
                .request = req,
                .path = path,
                .callback = std::move(cb), 
                .data = Vector<u8>{allocator_},
                .flags = {
                    .cancelled = 0,
                    .success = 0,
                    .anyThread = u8(anyThread ? 1 : 0),
                },
            });

            PROFILE_PLOT("IO tasks", i64(pending_.Size()));
        }

        cv_.Notify();

        return req;
    }

    void Cancel(RequestHandle request) override {
        if (request == 0) {
            return;
        }

        Lock lock{ mutex_ };
        if (activeRequest_ == request) {
            cancelActive_ = true;
            return;
        }

        for (auto& io : pending_) {
            if (io.request == request) {
                io.flags.cancelled = 1;
                return;
            }
        }

        for (auto& io : ready_) {
            if (io.request == request) {
                io.flags.cancelled = 1;
                return;
            }
        }
    }

    void CancelAll() override {
        {
            Lock lock{ mutex_ };
            cancelActive_ = true;

            for (auto& io : pending_) {
                io.flags.cancelled = 1;
            }

            for (auto& io : ready_) {
                io.flags.cancelled = 1;
            }
        }

        // TODO:
        //WaitIdle();
    }

private:
    struct IoTask {
        RequestHandle request{};
        Path path;
        Callback callback;
        Vector<u8> data;
        struct {
            u8 cancelled : 1;
            u8 success   : 1;
            u8 anyThread : 1;
        } flags{};
    };

    void IoThread() {
        IoTask task;

        while (running_) {
            { // Wait on work.
                Lock lock{ mutex_ };
                activeRequest_ = 0;
                if (pending_.Empty()) {
                    cv_.Wait(lock, [this] { return !running_ || !pending_.Empty(); });
                }

                if (!running_) {
                    break;
                }

                UGINE_ASSERT(!pending_.Empty());

                task = pending_.PopFront();
                PROFILE_PLOT("IO tasks", i64(pending_.Size()));

                if (task.flags.cancelled) {
                    continue;
                }

                activeRequest_ = task.request;
            }

            { // Do work.
                PROFILE_EVENT_N("IO operation");

                PROFILE_MESSAGE_DYN(task.path.Data(), task.path.String().Size());
                task.flags.success = Read(task.path, task.data);
                // TODO: Debug.
                //std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            // Handle result.

            {
                Lock lock{ mutex_ };
                if (cancelActive_) {
                    cancelActive_ = false;
                    task.flags.cancelled = 1;
                }
            }

            if (!task.flags.cancelled) {
                if (task.flags.anyThread) {
                    task.callback(task.data.ToSpan(), task.flags.success);
                } else {
                    Lock lock{ mutex_ };
                    ready_.PushBack(std::move(task));
                }
            }
        }
    }

    AllocatorRef allocator_;
    Path root_;

    std::atomic_bool running_{};

    // Uses either this:
    Scheduler* scheduler_{};
    UniquePtr<PinnedTask> schedulerTask_;
    // Or this:
    Thread thread_;

    Mutex mutex_;
    CondVar cv_;

    Vector<IoTask> pending_;
    Vector<IoTask> ready_;

    u32 counter_{};

    RequestHandle activeRequest_{};
    bool cancelActive_{};
};

UniquePtr<FileSystem> FileSystem::Create(const Path& root, IAllocator& allocator) {
    return MakeUnique<FileSystemDir>(allocator, root, allocator);
}

UniquePtr<FileSystem> FileSystem::Create(const Path& root, Scheduler& scheduler, u32 threadId, IAllocator& allocator) {
    return MakeUnique<FileSystemDir>(allocator, root, scheduler, threadId, allocator);
}

Path FileSystem::GetExecutablePath() {
#if defined(_WIN32)
    CHAR path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return Path{ path };
#endif

    return {};
}

Path FileSystem::GetUserDataPath() {
#if defined(_WIN32)
    CHAR appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath))) {
        return Path{ appDataPath };
    }
#endif

    return {};
}

bool FileSystem::Exists(const Path& path) {
    const auto dwAttrib{ GetFileAttributesA(path.Data()) };
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
}

bool FileSystem::CreateDirectories(const Path& path) {
    return SHCreateDirectoryExA(nullptr, path.Data(), nullptr) == ERROR_SUCCESS;
}

bool FileSystem::IsNewer(const Path& thisFile, const Path& thatFile) {
    // TODO: Check.

    auto src{ CreateFileA(thisFile.Data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr) };
    if (src == INVALID_HANDLE_VALUE) {
        return false;
    }

    auto dst{ CreateFileA(thatFile.Data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr) };
    if (dst == INVALID_HANDLE_VALUE) {
        CloseHandle(src);
        return false;
    }

    FILETIME srcCreate{}, srcAccess{}, srcWrite{};
    const auto resSrc{ GetFileTime(src, &srcCreate, &srcAccess, &srcWrite) };

    FILETIME dstCreate{}, dstAccess{}, dstWrite{};
    const auto resDst{ GetFileTime(dst, &dstCreate, &dstAccess, &dstWrite) };

    CloseHandle(src);
    CloseHandle(dst);

    if (resSrc == TRUE && resDst == TRUE) {
        const auto srcTime{ u64(srcWrite.dwHighDateTime) << 32 | u64(srcWrite.dwLowDateTime) };
        const auto dstTime{ u64(dstWrite.dwHighDateTime) << 32 | u64(dstWrite.dwLowDateTime) };

        return srcTime > dstTime;
    }

    return false;
}

bool FileSystem::Remove(const Path& path) {
    SHFILEOPSTRUCTA op = {
        .hwnd = nullptr,
        .wFunc = FO_DELETE,
        .pFrom = path.Data(),
        .pTo = "",
        .fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
        .fAnyOperationsAborted = false,
        .hNameMappings = 0,
        .lpszProgressTitle = "",
    };

    return SHFileOperationA(&op) == 0;
}

Path FileSystem::CurrentPath() {
    StaticString<MAX_PATH> buffer;
    buffer.Resize(GetCurrentDirectoryA(MAX_PATH, buffer.Data()));

    return Path{ buffer };
}

bool FileSystem::Rename(const Path& from, const Path& to) {
    return MoveFileExA(from.Data(), to.Data(), 0) == TRUE;
}

void Fill(const Path& path, DirectoryEntry& entry, const WIN32_FIND_DATAA& ff) {
    entry.path = path / ff.cFileName;
    entry.directory = ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    entry.regularFile = (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0; // TODO:
}

bool SkipSys(HANDLE handle, WIN32_FIND_DATAA& ff) {
    while (strcmp(ff.cFileName, ".") == 0 || strcmp(ff.cFileName, "..") == 0) {
        if (FindNextFileA(handle, &ff) == 0) {
            FindClose(handle);
            return false;
        }
    }
    return true;
}

DirectoryIterator::DirectoryIterator(const Path& path)
    : path_{ path } {
    WIN32_FIND_DATAA ff{};
    handle_ = FindFirstFileA((path / "*").Data(), &ff);

    if (handle_ != INVALID_HANDLE_VALUE) {
        if (SkipSys(handle_, ff)) {
            Fill(path_, entry_, ff);
        } else {
            handle_ = nullptr;
        }
    } else {
        handle_ = nullptr;
    }
}

DirectoryIterator::DirectoryIterator(const Path& path, const FileSystem& fs) {
    UGINE_ASSERT(false && "Not implemented");
}

DirectoryIterator::~DirectoryIterator() {
    if (handle_) {
        FindClose(handle_);
        handle_ = nullptr;
    }
}

DirectoryIterator& DirectoryIterator::operator++() {
    UGINE_ASSERT(handle_ && handle_ != INVALID_HANDLE_VALUE);

    WIN32_FIND_DATAA ff{};
    if (FindNextFileA(handle_, &ff) == 0) {
        handle_ = nullptr;
    } else if (SkipSys(handle_, ff)) {
        Fill(path_, entry_, ff);
    } else {
        handle_ = nullptr;
    }
    return *this;
}

} // namespace ugine