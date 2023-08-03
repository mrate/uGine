#include "FilesystemResourceBrowser.h"

#include <ugine/Profile.h>
#include <ugine/StringUtils.h>

#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/world/WorldSerializer.h>

#include "../EditorContext.h"

namespace ugine::ed {

void FilesystemResourceBrowser::FilterAll(bool all) {
    filter_ = all ? 0xffff : 0;
}

void FilesystemResourceBrowser::ResourceFilter(Filter& filter) {
    PROFILE_EVENT_NC("ResourceFilter", COLOR_EDITOR_PROFILE);

    auto check = [&](u8 type) {
        auto checked{ (filter & UGINE_BIT(type)) != 0 };
        if (ImGui::Checkbox(context_.ResourceTypes()[type].icon.Data(), &checked)) {
            if (ImGui::GetIO().KeyCtrl) {
                filter = UGINE_BIT(type);
            } else if (checked) {
                filter |= UGINE_BIT(type);
            } else {
                filter &= ~UGINE_BIT(type);
            }
        }
        ImGuiEx::ToolTip([&] { ImGui::TextUnformatted(context_.ResourceTypes()[type].type.Name()); });
    };

    for (u32 i{}; i < context_.ResourceTypes().Size(); ++i) {
        check(i);
        ImGui::SameLine();
    }

    bool all{ ugine::BitCount(filter_) == context_.ResourceTypes().Size() };
    if (ImGui::Checkbox("All", &all)) {
        FilterAll(all);
    }
}

void FilesystemResourceBrowser::RenderFiles() {
    PROFILE_EVENT_NC("ResourceFiles", COLOR_EDITOR_PROFILE);

    bool selectAll{};
    bool rename{};

    if (ImGui::IsWindowFocused()) {
        if (renaming_) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape) || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                renaming_ = false;
            }
        } else {
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
                selectAll = true;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_F2) && selection_.size() == 1) {
                renaming_ = true;
                rename = true;
            }
        }
    }

    auto state{ context_.Engine().GetState<GraphicsState>() };
    const auto& iconSize{ context_.ThumbnailSize() };

    for (const auto& file : files_) {
        if ((filter_ & UGINE_BIT(file.typeIndex)) == 0) {
            continue;
        }

        const auto filename{ file.path.Filename() };

        const auto& typeInfo{ context_.ResourceTypes()[file.typeIndex] };
        ImScope::Color textColor{ ImGuiCol_Text, typeInfo.color };

        bool selected{ selection_.contains(file.path) };

        auto texture{ context_.Thumbnails().Get(file.reference.id) };
        const auto changed{ ImGuiEx::ImageListItem(filename.Data(), ImGuiTextureHandle(texture, state->device), iconSize, true, &selected) };

        //if (selected && renaming_) {
        //    ImScope::StyleVar frameBorder{ ImGuiStyleVar_FrameBorderSize, 0.0f };

        //    if (rename) {
        //        strcpy(renameStr_, filename.c_str());
        //        ImGui::SetKeyboardFocusHere();
        //    }

        //    ImGui::TextUnformatted(typeInfo.icon.Data());
        //    ImGui::SameLine();
        //    if (ImGui::InputText("##rename_file", renameStr_, sizeof(renameStr_), ImGuiInputTextFlags_EnterReturnsTrue)) {
        //        renaming_ = false;
        //        context_.MoveResource(path_ / renameStr_, file.reference.id);
        //    }
        //} else {
        //    changed = ImGui::Selectable(filename.c_str(), &selected, ImGuiSelectableFlags_AllowDoubleClick) || changed;

        //    //ImGuiEx::ToolTip([&] {
        //    //    auto texture{ context_.Thumbnails().Get(file.reference.id) };
        //    //    if (texture) {
        //    //        ImGui::Image(ImGuiTextureHandle(texture, state->device), context_.ThumbnailSize());
        //    //    } else {
        //    //        ImGui::TextUnformatted("No preview");
        //    //    }
        //    //});
        //}

        if (!renaming_) {
            if (selectAll) {
                selection_.insert(std::make_pair(file.path, file.reference));
            } else if (changed) {
                const auto add{ ImGui::GetIO().KeyCtrl };
                if (selected) {
                    if (!add) {
                        selection_.clear();
                    }

                    selection_.insert(std::make_pair(file.path, file.reference));
                } else {
                    selection_.erase(file.path);
                }
            }

            if (changed && ImGui::IsMouseDoubleClicked(0)) {
                Enqueue(ResourceDoubleClicked{ file.reference });
            }

            if (ImGui::IsItemClicked(1)) {
                ugine::Vector<ResourceReference> selection;
                for (const auto& [_, ref] : selection_) {
                    selection.PushBack(ref);
                }
                if (!selection.Empty()) {
                    Enqueue(ResourceRightClicked{ selection });
                }
            }

            context_.DragAndDrop().BeginDrag([&] { return std::make_pair(file.reference, filename); });
        }
    }
}

void FilesystemResourceBrowser::SavePrefab(const GameObject& go) {
    auto [resource, path] = context_.CreateResource<WorldDescriptor>(path_, "Prefab");

    WorldSerializer serializer{};
    serializer.Serialize(*context_.ActiveWorld(), go, path);
}

void FilesystemResourceBrowser::Render() {
    if (needRefresh_) {
        needRefresh_ = false;

        Refresh();
    }

    if (ImGui::Begin(ICON_FA_FOLDER " Filesystem")) {
        const auto size{ ImGui::GetContentRegionAvail() };

        if (ImGui::BeginTable("##filesytem", 2, ImGuiTableFlags_Resizable, size)) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            if (context_.DirectorySelector().SelectDirectory("File", path_)) {
                SetPath(path_);
            }

            context_.DragAndDrop().DropResourceMulti([](const auto& id, const auto& type) { UGINE_DEBUG("Dropped resource."); });

            ImGui::TableNextColumn();
            ImGui::PushItemWidth(-1);

            {
                ImGui::TextUnformatted(ICON_FA_FOLDER_OPEN);

                // TODO: Reverse.
                auto p{ path_ };
                while (context_.IsInProjectDirectory(p)) {
                    auto dir{ p.Filename() };
                    ImGui::SameLine();
                    ImGui::TextUnformatted(dir.Data());
                    if (ImGui::IsItemClicked(0)) {
                        SetPath(p);
                    }
                    ImGui::SameLine();
                    ImGui::TextUnformatted(" < ");
                    p = p.ParentPath();
                }
            }

            { // Filter + reload toolbar.
                ResourceFilter(filter_);
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_REDO)) {
                    Refresh();
                }
                ImGuiEx::ToolTipText("Refresh directory");
            }

            if (ImGui::BeginListBox("##files", ImGui::GetContentRegionAvail())) {
                RenderFiles();
                ImGui::EndListBox();

                // Drag
                if (selection_.size() == 1) {
                    context_.DragAndDrop().BeginDrag(selection_.begin()->second, selection_.begin()->first.Filename());
                } else if (!selection_.empty()) {
                    context_.DragAndDrop().BeginDrag([&] {
                        ResourceReferences resources;
                        for (auto&& [_, ref] : selection_) {
                            resources.PushBack(ref);
                        }
                        return std::make_pair(resources, "Multiple resources");
                    });
                }

                // Drop
                ugine::GameObject go;
                if (context_.DragAndDrop().Drop<ugine::GameObject>(go)) {
                    SavePrefab(go);
                }
            }

            ImGui::PopItemWidth();
            ImGui::EndTable();
        }
    }

    ImGui::End();

    Update();
}

void FilesystemResourceBrowser::SetPath(const Path& path) {
    if (!path_.Empty()) {
        watcher_.Remove(path_);
    }

    path_ = path;
    selection_.clear();

    Refresh();

    if (autoRefresh_) {
        watcher_.Add(path_, [this](const auto& path, auto operation) { needRefresh_ = true; });
    }

    Emit(PathSelected{ path });
}

void FilesystemResourceBrowser::Select(const Path& path) {
    selection_.clear();

    for (const auto& file : files_) {
        if (file.path == path) {
            selection_.insert(std::make_pair(path, file.reference));
            break;
        }
    }
}

void FilesystemResourceBrowser::Refresh() {
    files_.Clear();

    for (const auto& [resourceId, entry] : context_.Engine().GetResources().RegisteredResources()) {
        const auto typeIndex{ context_.ResourceTypes().FindIf([&](const auto& type) { return type.type == entry.type; }) };
        if (typeIndex < 0) {
            continue;
        }

        if (context_.AbsolutePath(entry.path.ParentPath()) != path_) {
            continue;
        }

        files_.PushBack(File{
            .reference = ResourceReference{ resourceId, entry.type },
            .typeIndex = typeIndex,
            .name = entry.name,
            .path = entry.path,
        });
    }

    std::sort(files_.Begin(), files_.End(), [](const auto& f1, const auto& f2) { return f1.name < f2.name; });
}

} // namespace ugine::ed