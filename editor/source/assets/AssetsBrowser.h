#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include "../widgets/FilesystemResourceBrowser.h"

#include "AssetRenameDialog.h"

namespace ugine::ed {

class EditorContext;

class AssetsBrowser : public EditorTool {
public:
    explicit AssetsBrowser(EditorContext& context);

    void Render() override;

private:
    void OnResourceDoubleClicked(const FilesystemResourceBrowser::ResourceDoubleClicked& event);
    void OnResourceRightClicked(const FilesystemResourceBrowser::ResourceRightClicked& event);
    void OnPathSelected(const FilesystemResourceBrowser::PathSelected& event);
    void OnOpenProject(const OpenProjectEvent& event);
    void OnAssetModified(const AssetModifiedEvent& event);
    void OnLocateAsset(const LocateAssetEvent& event);
    void OnMoveAsset(const MoveAssetEvent& event);

    void Watch(Span<const ResourceReference> resources, bool watch);

    FilesystemResourceBrowser filesystemBrowser_;
    Path resourcesPath_;
    std::set<Path> watched_;
    AssetRenameDialog renameDialog_;
};

} // namespace ugine::ed