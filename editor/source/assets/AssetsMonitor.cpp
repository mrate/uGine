#include "AssetsMonitor.h"
#include "../EditorContext.h"

#include <ugine/FileWatcher.h>

#include <ugine/engine/gfx/ImGui.h>

namespace ugine::ed {

AssetsMonitor::AssetsMonitor(EditorContext& context)
    : EditorTool{ context, false } {
    auto& view{ context.MainMenu().Get("View") };

    view.AddAction([this] { return ImGui::Checkbox("Assets monitor", &visible_); });

    context_.Events().Connect<RegisterAssetWatcherEvent, &AssetsMonitor::OnRegisterAssetWatcher>(this);
    context_.Events().Connect<DeregisterAssetWatcherEvent, &AssetsMonitor::OnDeregisterAssetWatcher>(this);
}

void AssetsMonitor::AddFileWatch(const ResourceReference& resource, const Path& file) {
    watcher_.Add(file, [this, resource](FileWatcher::Operation operation, const Path& path) {
        switch (operation) {
        case FileWatcher::Operation::Modified:
            UGINE_DEBUG("Asset '{}' modified...", path.String());
            context_.Events().AssetModified(resource, path); // TODO: [PATH]
            break;
        default: break;
        }
    });
}

void AssetsMonitor::RemoveFileWatch(const ResourceReference& resource, const Path& file) {
    watcher_.Remove(file);
}

void AssetsMonitor::Render() {
    if (ImGui::Begin("Assets monitor", &visible_)) {
        for (const auto& [path, _] : watcher_.Watchers()) {
            ImGuiEx::Text(path);
        }
    }
    ImGui::End();
}

void AssetsMonitor::OnRegisterAssetWatcher(const RegisterAssetWatcherEvent& event) {
    AddFileWatch(event.resource, event.path);
}

void AssetsMonitor::OnDeregisterAssetWatcher(const DeregisterAssetWatcherEvent& event) {
    RemoveFileWatch(event.resource, event.path);
}

} // namespace ugine::ed