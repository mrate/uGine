#pragma once

#include "EditorEvents.h"
#include "EditorSettings.h"
#include "EditorTool.h"
#include "MetaFile.h"

#include "assets/EmbededAssets.h"

#include "preview/ResourceThumbnails.h"

#include "widgets/DirectoryTree.h"
#include "widgets/DragAndDrop.h"
#include "widgets/ImGuiEx.h"
#include "widgets/ImGuiScope.h"
#include "widgets/Menu.h"

#include <gfxapi/Types.h>

#include <ugine/File.h>
#include <ugine/Memory.h>
#include <ugine/StringUtils.h>
#include <ugine/Utils.h>

#include <ugine/engine/gfx/ImGui.h>

#include <imgui_internal.h>

#include <format>
#include <string>

namespace ugine::ed {

constexpr auto COLOR_EDITOR_PROFILE{ 0xffff7f28 };
constexpr auto WORLD_FLAGS_PREVIEW{ 0x1 };

struct GuiFlags {
    enum Flags {
        Selected = UGINE_BIT(0),
    };

    uint32_t flags{};
};

struct ResourceTypeInfo {
    ResourceType type{};
    String icon{};
    String extension{};
    ImVec4 color{};
};

class EditorContext {
public:
    explicit EditorContext(Engine& engine);
    ~EditorContext();

    void SaveSettings();

    void RunTextEditor(StringView file, std::optional<int> line = std::nullopt, std::optional<int> pos = std::nullopt);

    // Accessors;
    const Path& EditorExePath() const { return editorExePath_; }
    const Path& EditorPath() const { return editorPath_; }
    const Path& EditorUserPath() const { return editorUserPath_; }
    EditorSettings& Settings() { return settings_; }

    Engine& Engine() { return engine_; }
    gfxapi::Device& GfxDevice() { return gfxState_->device; }
    GraphicsState& GfxState() { return *gfxState_; }
    DragAndDrop& DragAndDrop() { return dragAndDrop_; }
    EmbededAssets& Assets() { return assets_; }
    IAllocator& Allocator() const { return engine_.GetAllocator(); }
    IAllocator& FrameAllocator(u32 threadNum = Scheduler::ThreadNum()) const { return engine_.FrameAllocator(threadNum); }

    EditorEvents& Events() { return events_; }

    ImGuiEx::Menu& MainMenu() { return mainMenu_; }
    ImGuiEx::ContextMenu& ContextMenu() { return contextMenu_; }

    const Vector<ResourceTypeInfo>& ResourceTypes() const { return registeredResources_; }
    World* ActiveWorld() { return world_; }

    const Vector<Path>& ShaderInludeDirs() const;

    bool IsPlaying() const { return isRunning_; }
    bool IsPaused() const { return isRunning_ && isPaused_; }

    // Project directory:
    void RefreshProjectDir();

    const Path& ProjectDirectory() const { return projectDir_; }
    void SetProjectDirectory(const Path& path) {
        projectDir_ = path;
        selectedAssetPath_ = path;
        RefreshProjectDir();
    }
    bool IsInProjectDirectory(const Path& path) const;
    Path RelativePath(const Path& path) const { return Path::RelativeDirectory(path, projectDir_); }
    Path AbsolutePath(const Path& path) const { return path.IsAbsolute() ? path : projectDir_ / path; }

    // Editor tools:
    void HandleEditorTools() {
        for (auto& tool : editorTools_) {
            if (tool->Visible()) {
                tool->Render();
            }
        }
    }
    void RegisterEditorTool(UniquePtr<EditorTool> tool) { editorTools_.PushBack(std::move(tool)); }

    // Resources:

    template <typename T> void RegisterResourceType(StringView icon, StringView extension, const ColorRGB& color) {
        registeredResources_.EmplaceBack(T::TYPE, icon, extension.Data(), ImVec4{ color.r, color.g, color.b, 1.0f });
    }

    template <typename T> const char* ResourceFileExtension() {
        const auto index{ registeredResources_.FindIf([](const auto& type) { return type.type == T::TYPE; }) };
        return index >= 0 ? registeredResources_[index].extension.Data() : "";
    }

    // Old interface:
    ResourceThumbnails& Thumbnails() { return resourceThumbnails_; }
    const ImVec2& ThumbnailSize() const { return thumbnailSize_; }
    void SetThumbnailSize(const ImVec2& size) {
        thumbnailSize_ = size;
        resourceThumbnails_.Clear();
    }

    DirectoryTree& DirectorySelector() { return dirTree_; }

    Path NewFilePath(const Path& targetPath, StringView fileName, StringView extension) {
        String file{ fileName };
        file.Append(extension);

        Path path{ targetPath / file };
        for (uint32_t index{}; FileSystem::Exists(path); ++index) {
            path = targetPath / std::format("{}_{}{}", fileName.Data(), index, extension.Data());
        }
        return RelativePath(path);
    }

    Path ResourceOrigin(const ResourceID& id) {
        const auto targetPath{ Engine().GetResources().ResourcePath(id) };
        if (targetPath.Empty()) {
            return {};
        }

        const auto meta{ MetaFile::OpenResource(targetPath) };
        return meta.Origin();
    }

    // TODO: [PATH]
    template <typename T>
    std::pair<ResourceHandle<T>, Path> CreateResource(
        const Path& targetDir, StringView fileName, std::optional<Path> originalPath = {}, std::optional<nlohmann::json> meta = {}) {
        // - Generate unique file name for new resource
        // - Register it in ResourceManager
        // - Create Meta file for new resource
        // - Notify listeners about new resource

        const auto resourcePath{ NewFilePath(targetDir, fileName, ResourceFileExtension<T>()) };

        auto resource{ engine_.GetResources().Create<T>() };

        UGINE_DEBUG("Creating {} {}...", T::TYPE.Name(), resource->Id().ToString());

        engine_.GetResources().RegisterResource(resource->Id(), resourcePath, T::TYPE);

        MetaFile::Create(resourcePath, resource->Id(), T::TYPE, originalPath, meta);

        events_.ResourceCreated(resource);

        return std::make_pair(resource, resourcePath);
    }

    template <typename T> void DeleteResource(ResourceHandle<T> resource) {
        UGINE_DEBUG("Deleting {} {}...", T::TYPE.Name(), resource->Id().ToString());

        events_.ResourceDeleted(ResourceReference{ resource->Id(), resource->Type() });

        const auto resourcePath{ engine_.GetResources().ResourcePath(resource->Id()) };
        MetaFile::Delete(resourcePath);
        engine_.GetResources().Delete<T>(resource->Id());
    }

    void DeleteResource(const ResourceReference& resource) {
        UGINE_DEBUG("Deleting {} {}...", resource.type.Name(), resource.id.ToString());

        events_.ResourceDeleted(resource);

        const auto resourcePath{ engine_.GetResources().ResourcePath(resource.id) };
        MetaFile::Delete(resourcePath);
        engine_.GetResources().Delete(resource.id, resource.type);
    }

    bool MoveResource(const Path& path, const ResourceID& id);

    void SelectGO(GameObject go) {
        DeselectGO();

        selectedGO_ = go;
        if (selectedGO_) {
            selectedGO_.GetOrCreateComponent<GuiFlags>().flags = GuiFlags::Selected;
            selectedGO_.SetStencil(1);
        }
    }

    void DeselectGO() {
        if (selectedGO_) {
            selectedGO_.RemoveComponent<GuiFlags>();
            selectedGO_.SetStencil(0);
        }
        selectedGO_ = {};
    }

    GameObject& SelectedGO() { return selectedGO_; }

    template <typename F> void Deferred(F f) { deferred_.PushBack(f); }
    void RunDeferred() {
        for (const auto& f : deferred_) {
            f();
        }
        deferred_.Clear();
    }

    const Path& SelectedAssetPath() const { return selectedAssetPath_; }

private:
    void OnWorldChanged(const WorldChangedEvent& event) { world_ = event.world; }
    void OnAssetPathChanged(const AssetPathChagnedEvent& event) { selectedAssetPath_ = event.path; }
    void OnResetThumbnail(const ResetThumbnailEvent& event);

    void OnStartWorldSimulation(const StartWorldSimulationEvent& event);
    void OnStopWorldSimulation(const StopWorldSimulationEvent& event);
    void OnPauseWorldSimulation(const PauseWorldSimulationEvent& event);

    EditorSettings settings_;

    ugine::Engine& engine_;
    GraphicsState* gfxState_{};

    Path editorExePath_{};
    Path editorPath_{};
    Path editorUserPath_{};
    Path editorSettingsPath_{};

    ResourceThumbnails resourceThumbnails_;
    ImVec2 thumbnailSize_{ 128, 128 };

    ed::DragAndDrop dragAndDrop_;
    EmbededAssets assets_;
    DirectoryTree dirTree_;

    Path selectedAssetPath_;

    ImGuiEx::Menu mainMenu_;
    ImGuiEx::ContextMenu contextMenu_;

    EditorEvents events_;

    Path projectDir_;
    GameObject selectedGO_;
    World* world_{};
    World* editorWorld_{};
    bool isRunning_{};
    bool isPaused_{};

    Vector<ResourceTypeInfo> registeredResources_;
    Vector<std::function<void()>> deferred_;

    Vector<UniquePtr<EditorTool>> editorTools_;
};

} // namespace ugine::ed