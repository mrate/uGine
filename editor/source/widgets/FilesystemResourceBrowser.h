#pragma once

#include <ugine/EventEmittor.h>
#include <ugine/engine/core/Resource.h>

#include <ugine/FileWatcher.h>

#include "../EditorEvents.h"

namespace ugine::ed {

class EditorContext;

class FilesystemResourceBrowser : public ugine::EventEmittor {
public:
    struct ResourceDoubleClicked {
        ResourceReference resource;
    };

    struct ResourceRightClicked {
        Vector<ResourceReference> resources;
    };

    struct PathSelected {
        Path path;
    };

    explicit FilesystemResourceBrowser(EditorContext& context)
        : context_{ context } {
        FilterAll(true);
    }

    void Render();

    void SetPath(const Path& path);
    void Select(const Path& path);

    void Refresh();

private:
    using Filter = u16;

    struct File {
        ResourceReference reference;
        int typeIndex{};
        String name;
        Path path;
    };

    void FilterAll(bool all);
    void ResourceFilter(Filter& resourceTypes);
    void RenderFiles();
    void SavePrefab(const GameObject& go);

    EditorContext& context_;
    Filter filter_;
    Path path_;

    std::unordered_map<Path, ResourceReference> selection_;
    ugine::Vector<File> files_;

    bool autoRefresh_{ true };
    ugine::FileWatcher watcher_;

    std::atomic_bool needRefresh_{};

    bool renaming_{};
    char renameStr_[256];
};

} // namespace ugine::ed