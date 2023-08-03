#include "DirectoryTree.h"
#include "../EditorContext.h"

#include <ugine/engine/gfx/ImGui.h>

#include <ugine/Path.h>
#include <ugine/Profile.h>

#include <filesystem>

namespace ugine::ed {

bool DirectoryTree::SelectDirectory(StringView label, Path& path) {
    PROFILE_EVENT_NC("SelectDirectory", COLOR_EDITOR_PROFILE);

    int id{};

    // TODO:
    //ImGui::Text("%s: project://%s", label.Data(), context_.RelativePath(path).Data());
    ImGui::Text("%s: project://%s", label.Data(), path.Data());
    ImGui::Separator();

    return dirCache_.Empty() ? false : Directory(*dirCache_[0], path, id);
}

bool DirectoryTree::Directory(DirectoryEntry& dir, Path& path, int& id) {
    PROFILE_EVENT_NC("Dir", COLOR_EDITOR_PROFILE);

    ImScope::Id _id{ &id };

    int flags{ ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth };
    if (id == 1) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    const char* icon{ ICON_FA_FOLDER };
    if (path == dir.path) {
        flags |= ImGuiTreeNodeFlags_Selected;
        icon = ICON_FA_FOLDER_OPEN;
    }
    if (dir.children.Empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    ImGui::TextUnformatted(icon);
    ImGui::SameLine();

    const auto expand{ ImGui::TreeNodeEx(dir.path.Filename().Data(), flags) };
    const auto nodeClicked{ ImGui::IsItemClicked(0) };

    bool childClicked{};
    if (expand) {
        for (const auto& child : dir.children) {
            if (Directory(*child, path, id)) {
                childClicked = true;
            }
        }
        ImGui::TreePop();
    }

    if (nodeClicked && !childClicked) {
        path = dir.path;
    }

    context_.DragAndDrop().DropResourceMulti([&](const auto& id, const auto& type) { context_.Events().MoveResource(dir.path, id); });

    return nodeClicked || childClicked;
}

void DirectoryTree::Refresh(Path& path) {
    dirCache_.Clear();

    dirCache_.PushBack(MakeUnique<DirectoryEntry>(IAllocator::Default(), path));
    RefreshDir(dirCache_.Back().Get(), path);
}

void DirectoryTree::RefreshDir(DirectoryEntry* dir, const Path& path) {
    for (auto it : DirectoryIterator{ path }) {
        if (it.IsDirectory()) {
            dir->children.PushBack(MakeUnique<DirectoryEntry>(IAllocator::Default(), it.Path()));
            RefreshDir(dir->children.Back().Get(), it.Path());
        }
    }
}

} // namespace ugine::ed