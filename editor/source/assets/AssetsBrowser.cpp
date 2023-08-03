#include "AssetsBrowser.h"
#include "../EditorContext.h"

#include <ugine/Profile.h>

namespace ugine::ed {

AssetsBrowser::AssetsBrowser(EditorContext& context)
    : EditorTool{ context }
    , filesystemBrowser_{ context }
    , renameDialog_{ context } {

    auto& view{ context_.MainMenu().Get("View") };
    view.AddAction([this] { return ImGui::Checkbox("Assets browser", &visible_); });

    filesystemBrowser_.Connect<FilesystemResourceBrowser::ResourceDoubleClicked, &AssetsBrowser::OnResourceDoubleClicked>(this);
    filesystemBrowser_.Connect<FilesystemResourceBrowser::ResourceRightClicked, &AssetsBrowser::OnResourceRightClicked>(this);
    filesystemBrowser_.Connect<FilesystemResourceBrowser::PathSelected, &AssetsBrowser::OnPathSelected>(this);

    context_.Events().Connect<OpenProjectEvent, &AssetsBrowser::OnOpenProject>(this);
    context_.Events().Connect<AssetModifiedEvent, &AssetsBrowser::OnAssetModified>(this);
    context_.Events().Connect<LocateAssetEvent, &AssetsBrowser::OnLocateAsset>(this);
    context_.Events().Connect<MoveAssetEvent, &AssetsBrowser::OnMoveAsset>(this);
}

void AssetsBrowser::Render() {
    filesystemBrowser_.Render();
}

void AssetsBrowser::OnResourceDoubleClicked(const FilesystemResourceBrowser::ResourceDoubleClicked& event) {
    const auto path{ context_.Engine().GetResources().ResourcePath(event.resource.id) };

    context_.Events().OpenTypelessResource(event.resource);
}

void AssetsBrowser::OnResourceRightClicked(const FilesystemResourceBrowser::ResourceRightClicked& event) {
    using namespace ImGuiEx;

    context_.ContextMenu().Open({
        MenuAction{
            .name = ICON_FA_RENAME_BOX " Rename",
            .callback =
                [this, event] {
                    if (event.resources.Size() == 1) {
                        renameDialog_.Rename(event.resources[0].id);
                        context_.Events().ShowModal(&renameDialog_);
                    }
                },
        },
        MenuAction{
            .name = ICON_FA_RELOAD " Reload",
            .callback = [resources = event.resources, this] { resources.ForEach([this](const auto& r) { context_.Events().ReloadResource(r); }); },
        },
        MenuAction{
            .render =
                [resources = event.resources, this] {
                    bool watched{};
                    if (resources.Size() == 1) {
                        const auto origin{ context_.ResourceOrigin(resources[0].id) };
                        watched = watched_.contains(origin);
                    }

                    if (ImGui::Checkbox(ICON_FA_BINOCULARS " Watch", &watched)) {
                        Watch(resources.ToSpan(), watched);
                        return true;
                    } else {
                        return false;
                    }
                },
        },
        MenuAction{},
        MenuAction{
            .name = ICON_FA_TRASH_CAN " Delete",
            .callback = [resources = event.resources, this] { resources.ForEach([this](const auto& r) { context_.DeleteResource(r); }); },
        },
    });
}

void AssetsBrowser::OnPathSelected(const FilesystemResourceBrowser::PathSelected& event) {
    context_.Events().AssetPathSelected(event.path);
}

void AssetsBrowser::OnOpenProject(const OpenProjectEvent& event) {
    resourcesPath_ = event.path;
    filesystemBrowser_.SetPath(resourcesPath_);
}

void AssetsBrowser::OnAssetModified(const AssetModifiedEvent& event) {
    context_.Events().ReloadResource(event.resource);
}

void AssetsBrowser::OnLocateAsset(const LocateAssetEvent& event) {
    const auto path{ context_.Engine().GetResources().ResourcePath(event.id) };

    if (path.Empty()) {
        return;
    }

    filesystemBrowser_.SetPath(context_.AbsolutePath(path.ParentPath()));
    filesystemBrowser_.Select(path);
}

void AssetsBrowser::Watch(Span<const ResourceReference> resources, bool watch) {
    for (const auto& resource : resources) {
        const auto origin{ context_.ResourceOrigin(resource.id) };
        bool watched{ watched_.contains(origin) };

        ImScope::Disabled disabled{ origin.Empty() };

        if (watch) {
            watched_.insert(origin);
            context_.Events().RegisterAssetWatcher(resource, origin);
        } else {
            watched_.erase(origin);
            context_.Events().DeregisterAssetWatcher(resource, origin);
        }
    }
}

void AssetsBrowser::OnMoveAsset(const MoveAssetEvent& event) {
    const auto path{ context_.Engine().GetResources().ResourcePath(event.id) };

    context_.MoveResource(event.destination / path, event.id);
}

} // namespace ugine::ed