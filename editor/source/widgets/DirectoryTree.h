#pragma once

#include <ugine/Memory.h>
#include <ugine/Path.h>
#include <ugine/String.h>
#include <ugine/Vector.h>

namespace ugine {
class Path;
}

namespace ugine::ed {

class EditorContext;

class DirectoryTree {
public:
    explicit DirectoryTree(EditorContext& context)
        : context_{ context } {}

    bool SelectDirectory(StringView label, Path& path);

    void Refresh(Path& path);

private:
    struct DirectoryEntry {
        Path path;
        Vector<UniquePtr<DirectoryEntry>> children;
    };

    void RefreshDir(DirectoryEntry* dir, const Path& path);
    bool Directory(DirectoryEntry& dir, Path& path, int& id);

    EditorContext& context_;
    Vector<UniquePtr<DirectoryEntry>> dirCache_;
};

} // namespace ugine::ed