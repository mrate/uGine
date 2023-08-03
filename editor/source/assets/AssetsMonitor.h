#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include <ugine/FileWatcher.h>

namespace ugine::ed {

class AssetsMonitor : public EditorTool {
public:
    explicit AssetsMonitor(EditorContext& context);

    void AddFileWatch(const ResourceReference& resource, const Path& file);
    void RemoveFileWatch(const ResourceReference& resource, const Path& file);

    void Render() override;

private:
    void OnRegisterAssetWatcher(const RegisterAssetWatcherEvent& event);
    void OnDeregisterAssetWatcher(const DeregisterAssetWatcherEvent& event);

    FileWatcher watcher_;
};

} // namespace ugine::ed