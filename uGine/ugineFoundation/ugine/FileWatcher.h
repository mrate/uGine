#pragma once

#include <ugine/Memory.h>
#include <ugine/Path.h>

#pragma warning(push)
#pragma warning(disable : 4068)
#include <FileWatch.hpp>
#pragma warning(pop)

#include <filesystem>
#include <map>
#include <string>

namespace ugine {

class FileWatcher {
public:
    enum class Operation {
        Added,
        Removed,
        Modified,
    };

    using Watcher = filewatch::FileWatch<std::string>;

    explicit FileWatcher(IAllocator& allocator = IAllocator::Default())
        : allocator_{ allocator } {}

    template <typename T> void Add(const Path& file, T&& t) {
        watchers_.insert(std::make_pair(file, MakeUnique<Watcher>(allocator_, file.Data(), [file, t](const auto& /*path*/, const filewatch::Event changeType) {
            Operation operation{};
            switch (changeType) {
            case filewatch::Event::added: operation = Operation::Added; break;
            case filewatch::Event::removed: operation = Operation::Removed; break;
            case filewatch::Event::modified: operation = Operation::Modified; break;
            default: return;
            }

            t(operation, file);
        })));
    }

    void Remove(const Path& file) { watchers_.erase(file); }

    const std::map<Path, UniquePtr<Watcher>>& Watchers() const { return watchers_; }

private:
    AllocatorRef allocator_;
    std::map<Path, UniquePtr<Watcher>> watchers_;
};

} // namespace ugine
