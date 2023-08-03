#include "EditorContext.h"

#include <ugine/Os.h>

#include <ugine/engine/gfx/Shapes.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>
#include <ugine/engine/script/Lua.h>
#include <ugine/engine/script/ScriptState.h>
#include <ugine/engine/world/WorldManager.h>

#include "widgets/ImGuiScope.h"

namespace ugine::ed {

void RegisterLuaState(lua_State* state, EditorContext* editor) {
    using namespace luabridge;

    getGlobalNamespace(state)
        .beginNamespace("ugine")

        .beginClass<EditorContext>("Editor")
        .addFunction("activeWorld", &EditorContext::ActiveWorld)
        .addFunction("select", &EditorContext::SelectGO)
        .addFunction("deselect", &EditorContext::DeselectGO)
        .addFunction("selected", &EditorContext::SelectedGO)
        .endClass()

        .endNamespace();

    getGlobalNamespace(state).beginNamespace("ugine").addProperty("editor", [editor] { return editor; }).endNamespace();
}

EditorContext::EditorContext(ugine::Engine& engine)
    : engine_{ engine }
    , gfxState_{ engine.GetState<GraphicsState>() }
    , editorExePath_{ FileSystem::GetExecutablePath() }
    , editorPath_{ editorExePath_.ParentPath() }
    , editorUserPath_{ FileSystem::GetUserDataPath() / "ugineEditor" }
    , editorSettingsPath_{ editorUserPath_ / "settings.json" }
    , resourceThumbnails_{ engine }
    , dragAndDrop_{ engine.GetResources() }
    , assets_{ *this }
    , dirTree_{ *this } {

    events_.Connect<WorldChangedEvent, &EditorContext::OnWorldChanged>(this);
    events_.Connect<AssetPathChagnedEvent, &EditorContext::OnAssetPathChanged>(this);

    events_.Connect<StartWorldSimulationEvent, &EditorContext::OnStartWorldSimulation>(this);
    events_.Connect<StopWorldSimulationEvent, &EditorContext::OnStopWorldSimulation>(this);
    events_.Connect<PauseWorldSimulationEvent, &EditorContext::OnPauseWorldSimulation>(this);

    events_.Connect<ResetThumbnailEvent, &EditorContext::OnResetThumbnail>(this);

    selectedAssetPath_ = ProjectDirectory();

    if (auto scriptState{ engine_.GetState<ScriptState>() }; scriptState) {
        RegisterLuaState(scriptState->LuaState(), this);
    }

    if (!FileSystem::Exists(editorUserPath_)) {
        FileSystem::CreateDirectories(editorUserPath_);
    } else if (FileSystem::Exists(editorSettingsPath_)) {
        UGINE_INFO("Loading editor settings '{}'", editorSettingsPath_.String());

        settings_.Load(editorSettingsPath_);
    }
}

EditorContext::~EditorContext() {
    SaveSettings();
}

void EditorContext::SaveSettings() {
    settings_.Save(editorSettingsPath_);
}

bool EditorContext::IsInProjectDirectory(const Path& path) const {
    auto parent{ path };
    while (parent.HasParentPath() && parent != parent.ParentPath()) {
        if (parent == projectDir_) {
            return true;
        }
        parent = parent.ParentPath();
    }
    return false;
}

const Vector<Path>& EditorContext::ShaderInludeDirs() const {
    static const Vector<Path> includes = {
        // TODO:
        //editorExePath_.parent_path() / "shaders",
        editorExePath_.ParentPath() / ".." / ".." / ".." / ".." / "uGine" / "ugineEngine" / "ugine" / "engine" / "shaders",
    };

    for (const auto& include : includes) {
        if (!FileSystem::Exists(include)) {
            UGINE_ERROR("Shader include path not exists");
            UGINE_ASSERT(false);
        }
    }

    return includes;
}

void EditorContext::RefreshProjectDir() {
    dirTree_.Refresh(projectDir_);
}

bool EditorContext::MoveResource(const Path& targetResourcePath, const ResourceID& id) {
    const auto resourcePath{ AbsolutePath(engine_.GetResources().ResourcePath(id)) };
    if (resourcePath.Empty()) {
        UGINE_WARN("Cannot move, path not found for resource {}", id.ToString());
        return false;
    }

    if (resourcePath == targetResourcePath) {
        return false;
    }

    const auto metaPath{ MetaFile::MetaPathFromResourcePath(resourcePath) };
    const auto targetMetaPath{ MetaFile::MetaPathFromResourcePath(targetResourcePath) };

    if (FileSystem::Exists(targetResourcePath) || FileSystem::Exists(targetMetaPath)) {
        UGINE_WARN("Target file already exists, skipping.");
        return false;
    }

    UGINE_INFO("Moving resource '{}': {} => {}", id.ToString(), resourcePath.String(), targetResourcePath.String());

    FileSystem::Rename(resourcePath, targetResourcePath);
    FileSystem::Rename(metaPath, targetMetaPath);

    engine_.GetResources().SetResourcePath(id, targetResourcePath);

    return true;
}

void EditorContext::OnResetThumbnail(const ResetThumbnailEvent& event) {
    resourceThumbnails_.Invalidate(event.id);
}

void EditorContext::OnStartWorldSimulation(const StartWorldSimulationEvent& event) {
    if (isRunning_) {
        return;
    }

    DeselectGO();
    isRunning_ = true;
    isPaused_ = false;

    editorWorld_ = world_;

    //world_->SetSimulating(true);
    auto newWorld{ engine_.GetWorldManager().Clone(*world_) };
    newWorld->SetSimulating(true);
    newWorld->SetRendering(true);
    events_.ChangeWorld(newWorld);
}

void EditorContext::OnStopWorldSimulation(const StopWorldSimulationEvent& event) {
    if (!isRunning_) {
        return;
    }

    DeselectGO();
    isRunning_ = false;
    isPaused_ = false;

    //world_->SetSimulating(false);
    auto oldWorld{ world_ };
    events_.ChangeWorld(editorWorld_);
    engine_.GetWorldManager().DestroyWorld(oldWorld);
}

void EditorContext::OnPauseWorldSimulation(const PauseWorldSimulationEvent& event) {
    ActiveWorld()->SetSimulating(!event.pause);
    isPaused_ = event.pause;
}

void EditorContext::RunTextEditor(StringView file, std::optional<int> line, std::optional<int> pos) {
    // TODO: Settings.
    Vector<String> args;
    if (line && pos) {
        args.PushBack("-g");
        args.PushBack(std::format("{}:{}:{}", file.Data(), *line, *pos).c_str());
    } else if (line) {
        args.PushBack("-g");
        args.PushBack(std::format("{}:{}", file.Data(), *line).c_str());
    } else {
        args.PushBack(file);
    }

    RunProcess("\"c:\\Program Files\\Microsoft VS Code\\Code.exe\"", args.ToSpan());
}

} // namespace ugine::ed