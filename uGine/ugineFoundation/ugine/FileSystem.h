#pragma once

#include <ugine/Delegate.h>
#include <ugine/Memory.h>
#include <ugine/Path.h>
#include <ugine/String.h>
#include <ugine/Vector.h>

namespace ugine {

class Scheduler;
class FileSystem;

struct DirectoryEntry {
    bool IsDirectory() const { return directory; }
    bool IsRegularFile() const { return regularFile; }
    const Path& Path() const { return path; }

    ugine::Path path;
    bool directory{};
    bool regularFile{};
};

class DirectoryIterator {
public:
    DirectoryIterator() = default;
    explicit DirectoryIterator(const Path& path);
    DirectoryIterator(const Path& path, const FileSystem& fs);

    DirectoryIterator(const DirectoryIterator& other)
        : DirectoryIterator{ other.path_ } {
        UGINE_ASSERT(path_.String() == other.path_.String());
    }
    DirectoryIterator& operator=(const DirectoryIterator&) = delete;

    DirectoryIterator(DirectoryIterator&& other) noexcept
        : handle_{ std::exchange(other.handle_, nullptr) }
        , entry_{ std::move(other.entry_) } {}
    DirectoryIterator& operator=(DirectoryIterator&& other) noexcept {
        handle_ = std::exchange(other.handle_, nullptr);
        entry_ = std::move(other.entry_);
        return *this;
    }

    ~DirectoryIterator();

    DirectoryIterator& operator++();

    DirectoryEntry& operator*() { return entry_; }
    DirectoryEntry& operator->() { return entry_; }

    friend bool operator==(const DirectoryIterator& a, const DirectoryIterator& b) { return a.handle_ == b.handle_; }
    friend bool operator!=(const DirectoryIterator& a, const DirectoryIterator& b) { return !(a == b); }

private:
    Path path_{};
    void* handle_{};
    DirectoryEntry entry_{};
};

class FileSystem {
public:
    using Callback = Delegate<void(Span<const u8> /*data*/, bool /*success*/)>;
    using RequestHandle = u32;

    static bool Exists(const Path& path);
    static bool CreateDirectories(const Path& path);
    static bool IsNewer(const Path& thisFile, const Path& thatFile);
    static bool Remove(const Path& path);
    static bool Rename(const Path& from, const Path& to);
    static Path CurrentPath();    

    static Path GetExecutablePath();
    static Path GetUserDataPath();

    static UniquePtr<FileSystem> Create(const Path& root, IAllocator& allocator = IAllocator::Default());
    static UniquePtr<FileSystem> Create(const Path& root, Scheduler& scheduler, u32 threadId, IAllocator& allocator = IAllocator::Default());

    virtual ~FileSystem() = default;

    virtual bool ReadOnly() const = 0;

    virtual bool Read(const Path& path, Vector<u8>& data) = 0;
    virtual bool Write(const Path& path, Span<const u8> data) = 0;

    virtual RequestHandle ReadAsync(const Path& path, Callback cb, bool anyThread = false) = 0;
    virtual void Cancel(RequestHandle request) = 0;
    virtual void CancelAll() = 0;

    virtual void SyncPoint() = 0;
};

} // namespace ugine

namespace std {
UGINE_FORCE_INLINE ugine::DirectoryIterator begin(const ugine::DirectoryIterator& it) noexcept {
    return it;
}

UGINE_FORCE_INLINE ugine::DirectoryIterator end(const ugine::DirectoryIterator&) noexcept {
    return {};
}
} // namespace std