#include "ImportTextureWindow.h"

#include "../EditorContext.h"
#include "../EditorEvents.h"

#include "../model/MeshImporter.h"

#include "../widgets/ImGuiEx.h"
#include "../widgets/ImGuiScope.h"

#include "../platform/FileDialog.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/ImGui.h>

namespace ugine::ed {

namespace {
    const Vector<String> TEXTURE_FILES_FILTER{ "Texture Files", "*.png;*.ktx;*.jpg", "All files", "*.*" };
}

ImportTextureWindow::ImportTextureWindow(EditorContext& context)
    : ModalButtonWindow{ context }
    , context_{ context } {
    context_.Events().Connect<ReloadResourceEvent, &ImportTextureWindow::OnReloadResource>(this);
}

void ImportTextureWindow::Init() {
    try {
        auto files{ OpenFileDialog(context_.Engine().GetPlatform().GetNativeHandle(), TEXTURE_FILES_FILTER, true) };
        if (files.Empty()) {
            context_.Events().CloseModal(this);
            return;
        }

        sourcePaths_ = std::move(files);
        if (sourcePaths_.Size() == 6) {
            ReorderCubeMap();
        } else {
            isCubeMap_ = false;
        }

        SetRightButtons({ Button{
            .enabled = true,
            .name = ICON_FA_FOLDER_OPEN " Import",
            .func = [this] { ImportTextures(); },
        } });
    } catch (const std::exception& ex) {
        Emit(ErrorEvent{ ex.what() });
    }
}

void ImportTextureWindow::Show() {
    BeginContent();
    //for (const auto& sourcePath : sourcePaths_) {
    //    ImGui::Text("Source: %s", sourcePath.string().c_str());
    //}

    {
        ImScope::Disabled disabled{ sourcePaths_.Size() != 6 };
        ImGui::Checkbox("Import as cubemap", &isCubeMap_);
        if (isCubeMap_) {
            CubeMapOrdering();
        }
    }

    if (sourcePaths_.Size() != 6) {
        ImGuiEx::ToolTipText("Needs exactly 6 textures to create cubemap.");
    }

    context_.DirectorySelector().SelectDirectory("Destination:", targetPath_);
    EndContent();

    Buttons();

    ImGui::Separator();
}

bool ImportTextureWindow::ImportTextures() {
    bool error{};

    nlohmann::json meta{};
    auto& sources{ meta["sourceFiles"] };
    for (const auto& path : sourcePaths_) {
        sources.push_back(path.Data());
    }
    meta["cubeMap"] = isCubeMap_;

    if (isCubeMap_) {
        const auto name{ std::format("{}_cubemap", sourcePaths_[0].Stem()) };
        auto [resource, path] = context_.CreateResource<Texture>(targetPath_, name.c_str(), sourcePaths_[0], meta);

        Image cubeMap{};
        if (!Image::FromFile(sourcePaths_[0], cubeMap)) {
            Emit(ErrorEvent{ "Failed to load cubemap image" });
            return false;
        }

        cubeMap.SetCubemap(true);

        for (int i{ 1 }; i < sourcePaths_.Size(); ++i) {
            if (!cubeMap.AddLayerFromFile(i, sourcePaths_[i])) {
                Emit(ErrorEvent{ "Failed to load cubemap layer" });
                return false;
            }
        }

        if (!cubeMap.Save(path)) {
            Emit(ErrorEvent{ std::format("Failed to save image: {}", path.String()).c_str() });
            return false;
        }

    } else {
        for (const auto& file : sourcePaths_) {
            const auto name{ file.Stem() };
            auto [resource, path] = context_.CreateResource<Texture>(targetPath_, name, file, meta);

            Image image;
            if (!Image::FromFile(file, image)) {
                Emit(ErrorEvent{ "Failed to load image" });
                error = true;
                continue;
            }

            if (!image.Save(path)) {
                Emit(ErrorEvent{ std::format("Failed to save image: {}", path.String()).c_str() });
                continue;
            }
        }
    }

    if (!error) {
        context_.Events().CloseModal(this);
    }

    return error;
}

void ImportTextureWindow::CubeMapOrdering() {
    const auto size{ ImGui::GetContentRegionAvail() };

    const auto col{ 90 };
    const auto row{ 40 };

    if (!ImGui::BeginChildFrame(1, ImVec2{ size.x, 3 * row })) {
        return;
    }

    const ImVec2 pos[] = {
        { 2 * col, row },
        { 0, row },
        { col, 0 },
        { col, 2 * row },
        { col, row },
        { 3 * col, row },
    };

    const auto labels{ ImGui::GetIO().KeyCtrl };
    const char* face[] = { "x+", "x-", "y+", "y-", "z-", "z+" };

    for (int n{}; n < 6; ++n) {
        const auto fileName{ sourcePaths_[n].Filename() };

        ImScope::Id id{ n };

        ImGui::SetCursorPos(pos[n]);

        ImGui::Button(labels ? face[n] : fileName.Data(), ImVec2(80, 30));
        ImGuiEx::ToolTipText(fileName.Data());

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("CUBE_MAP_CELL", &n, sizeof(int));
            ImGui::TextUnformatted(sourcePaths_[n].Filename().Data());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CUBE_MAP_CELL")) {
                UGINE_ASSERT(payload->DataSize == sizeof(int));

                const int payload_n{ *(const int*)payload->Data };
                std::swap(sourcePaths_[n], sourcePaths_[payload_n]);
            }
            ImGui::EndDragDropTarget();
        }
    }

    ImGui::EndChildFrame();
}

void ImportTextureWindow::ReorderCubeMap() {
    static const std::vector<std::vector<std::string>> patterns = {
        { "right", "x+", "+x" },
        { "left", "x-", "-x" },
        { "top", "up", "y+", "+y" },
        { "bottom", "down", "y+", "+y" },
        { "back", "z-", "-z" },
        { "front", "z+", "+z" },
    };

    std::array<int, 6> index = { 0, 1, 2, 3, 4, 5 };

    for (int j{}; j < 6; ++j) {
        std::string name{ sourcePaths_[j].Filename().Data() };
        std::for_each(name.begin(), name.end(), [](char& v) { return std::tolower(v); });

        for (int i{}; i < 6; ++i) {
            const auto pattern{ patterns[i] };

            bool found{};
            for (auto& p : pattern) {
                if (name.find(p) != std::string::npos) {
                    if (index[j] != i) {
                        std::swap(index[j], index[i]);
                    }
                    found = true;
                    break;
                }
            }

            if (found) {
                break;
            }
        }
    }

    Vector<Path> tmp{ sourcePaths_ };
    for (int i{}; i < 6; ++i) {
        sourcePaths_[i] = tmp[index[i]];
    }
}

void ImportTextureWindow::ReloadTextures(const ResourceID& id) {
    // TODO:

    //targetPath_ = context_.Engine().GetResources().ResourcePath(id);

    //auto metaFile{ MetaFile::OpenResource(targetPath_) };
    //const auto& meta{ metaFile.Meta() };

    //if (meta.empty()) {
    //    UGINE_WARN("Unable to reload texture '{}', missing meta", id.ToString());
    //    return;
    //}

    //sourcePaths_.clear();
    //isCubeMap_ = meta.value<bool>("cubeMap", false);
    //for (const auto& path : meta["sourceFiles"]) {
    //    sourcePaths_.push_back(path);
    //}

    //if (isCubeMap_ && sourcePaths_.size() != 6) {
    //    UGINE_ERROR("Invalid cubemap: {} textures provided", sourcePaths_.size());
    //    return;
    //}

    //if (!isCubeMap_ && sourcePaths_.size() != 1) {
    //    UGINE_ERROR("Invalid texture source provided: {}", sourcePaths_.size());
    //    return;
    //}

    //// Reimport texture -> backup original resource file.
    //auto targetPathBak{ targetPath_ };
    //targetPathBak.replace_extension(".bak");

    //std::filesystem::rename(targetPath_, targetPathBak);

    //if (ImportTextures()) {
    //    // Delete backup.
    //    std::filesystem::remove(targetPathBak);

    //    context_.Engine().GetResources().Reload<Texture>(id);
    //} else {
    //    // Restore backup.

    //    std::filesystem::remove(targetPath_);
    //    std::filesystem::rename(targetPathBak, targetPath_);
    //}
}

void ImportTextureWindow::OnReloadResource(const ReloadResourceEvent& event) {
    if (event.resource.type == Texture::TYPE) {
        ReloadTextures(event.resource.id);
    }
}

} // namespace ugine::ed