#include "EditorContext.h"
#include "EditorScene.h"

#include "widgets/ImGuiEx.h"
#include "widgets/ImGuiScope.h"
#include "widgets/Menu.h"

#include "Window/Window.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/engine/System.h>
#include <ugine/engine/input/InputState.h>
#include <ugine/engine/system/Event.h>
#include <ugine/engine/world/WorldManager.h>

#include <ugineEditorConfig.h>

#include <gfxapi/Device.h>

#include <ugine/EventEmittor.h>
#include <ugine/FileSystem.h>
#include <ugine/Log.h>
#include <ugine/Profile.h>

#include <ImGuizmo.h>

#include <format>
#include <fstream>
#include <iostream>

namespace ugine::ed {

void InitEditorStyle();

class Editor : public System {
public:
    Editor(Engine& engine)
        : System{ engine }
        , context_{ engine }
        , thumbnailCreator_{ context_ }
        , messageWindow_{ context_ } {

        LoggerCallback callback{};
        callback.Connect<&Editor::Log>(this);
        engine.SetLogger(callback);

        engine.GetWorldManager().Connect<WorldManager::WorldCreatedEvent, &Editor::OnWorldCreated>(this);
    }

    ~Editor() { exit_ = true; }

private:
    std::atomic_bool exit_{};

    void OnWorldCreated(const WorldManager::WorldCreatedEvent& event) {
        // TODO: Only actual game worlds no preview worlds.
        if (event.world->GetUserFlags() == 0) {
            event.world->AddScene(MakeUnique<EditorScene>(GetEngine().GetAllocator(), GetEngine(), *event.world, context_, GetEngine().GetAllocator()));
        }
    }

    void Log(LogLevel level, u64 millis, Span<const char> message) {
        if (!exit_) {
            context_.Events().Log(level, millis, message);
        }
    }

    void Initialize() override {
        InitEditorStyle();

        // Menu.
        CreateMenu();
        EditorToolset::RegisterEditor(context_);

        context_.Events().Connect<ErrorEvent, &Editor::OnError>(this);
        context_.Events().Connect<ShowModalWindowEvent, &Editor::OnShowModal>(this);
        context_.Events().Connect<CloseModalWindowEvent, &Editor::OnCloseModal>(this);

        context_.Events().Connect<OpenProjectEvent, &Editor::OnOpenProject>(this);

        // Init.
        context_.Events().OpenProject(FileSystem::CurrentPath());
    }

    void OnOpenProject(const OpenProjectEvent& event) {
        UGINE_INFO("Opening project '{}'...", event.path.String());

        context_.SetProjectDirectory(event.path);
        ScanDirectory(context_.ProjectDirectory());
    }

    void ScanDirectory(const Path& dir, bool loadResources = false) {
        UGINE_TRACE("Scanning project directory {}...", dir.String());

        for (const auto it : DirectoryIterator{ dir }) {
            if (it.IsDirectory()) {
                ScanDirectory(it.Path(), loadResources);
            } else if (it.IsRegularFile() && it.Path().Extension() == ".meta") {
                AddResourceFile(it.Path(), loadResources);
            }
        }
    }

    void AddResourceFile(const Path& metaFile, bool loadResources) {
        try {
            MetaFile meta{ metaFile };
            if (FileSystem::Exists(meta.ResourcePath())) {
                GetEngine().GetResources().RegisterResource(meta.Id(), context_.RelativePath(meta.ResourcePath()), meta.Type());
            } else {
                UGINE_ERROR("Meta file '{}' for non-existing resource '{}', removing.", metaFile.String(), meta.ResourcePath().String());

                FileSystem::Remove(metaFile);
            }

            if (!loadResources) {
                return;
            }

            //switch (meta.Type()) {
            //    using enum MetaFile::ResourceType;

            //case Texture: GetEngine().GetResources().Get<ugine::Texture>(meta.Id()); break;
            //case Model: GetEngine().GetResources().Get<ugine::Model>(meta.Id()); break;
            //case Material: GetEngine().GetResources().Get<ugine::Material>(meta.Id()); break;
            //case Animation: GetEngine().GetResources().Get<ugine::Animation>(meta.Id()); break;
            //case WorldDescriptor: GetEngine().GetResources().Get<ugine::WorldDescriptor>(meta.Id()); break;
            //}
        } catch (const std::exception& ex) {
            // TODO:
            UGINE_ERROR("Failed to add resource file '{}': {}", metaFile.String(), ex.what());
        }
    }

    void EarlyUpdate() override {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
    }

    void Update() override {
        if (GetEngine().IsQuitting()) {
            return;
        }

        if (auto input{ GetEngine().GetState<InputState>() }; input) {
            if (!modalWindows_.Empty() && input->KeyPressed(InputKeyCode::Key_Escape)) {
                CloseModal(modalWindows_.Back());
            }

            if (input->KeyPressed(InputKeyCode::Key_F1)) {
                GetEngine().GetPlatform().SetMouseCapture(true);
            }

            if (input->KeyPressed(InputKeyCode::Key_F2)) {
                GetEngine().GetPlatform().SetMouseCapture(false);
            }
        }

        {
            auto cmd{ GetEngine().GetState<GraphicsState>()->device.BeginCommandList() };

            auto& wm{ GetEngine().GetWorldManager() };
            wm.ForEachScene<EditorScene>([&](EditorScene& scene) {
                scene.Update();
                scene.Render(*cmd);
            });
        }

        thumbnailCreator_.Update();
        Gui();
    }

    bool HandleSystemEvent(const SystemEvent& event) override {
        //if (event.type == SystemEvent::Type::WindowResized) {
        //    Resize(event.resize.width, event.resize.height);
        //}

        return false;
    }

    void Gui() {
        PROFILE_EVENT_NC("Editor UI", COLOR_EDITOR_PROFILE);

        const auto region{ ImGui::GetContentRegionAvail() };

        if (ImGui::BeginMainMenuBar()) {
            const auto pos{ ImGui::GetCursorPos() };
            const auto size{ ImGui::GetMainViewport()->Size };

            context_.MainMenu().Render();

            { // FPS.
                static const float width{ ImGui::CalcTextSize(ICON_FA_TIMER " FPS: 999999").x };
                ImGui::SetCursorPos(ImVec2{ pos.x + size.x - width, pos.y });
                ImGui::Text(ICON_FA_TIMER " FPS: %.0f", GetEngine().Fps());
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        ShowModalWindow();

        if (imGuiDemoVisible_) {
            ImGui::ShowDemoWindow(&imGuiDemoVisible_);
        }

        context_.HandleEditorTools();
        context_.ContextMenu().Render();
        context_.RunDeferred();

        while (context_.Events().Trigger()) {
        }
    }

    void CreateMenu() {
        // To maintain menu ordering.
        context_.MainMenu().Get("File");
        context_.MainMenu().Get("Import");
        context_.MainMenu().Get("View");
        context_.MainMenu().Get("Debug");

        auto& debug{ context_.MainMenu().Get("Debug") };
        debug.AddSeparator();
        debug.AddAction("Show ImGui demo", [this] { imGuiDemoVisible_ = true; });
    }

    void ShowModalWindow() {
        PROFILE_EVENT_NC("ModalWindows", COLOR_EDITOR_PROFILE);

        if (modalWindows_.Empty()) {
            return;
        }

        auto& window{ modalWindows_.Back() };
        ImGui::OpenPopup(window->Name());

        const auto center{ ImGui::GetMainViewport()->GetCenter() };
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        bool open{ true };
        if (ImGui::BeginPopupModal(window->Name(), &open, window->Flags())) {
            try {
                window->Show();
            } catch (const std::exception& ex) {
                ShowMessage(MessageWindow::Type::Error, ex.what());
            }
            ImGui::EndPopup();
        }

        if (!open) {
            CloseModal(window);
        }
    }

    void OnShowModal(const ShowModalWindowEvent& event) { ShowModal(event.window); }
    void OnCloseModal(const CloseModalWindowEvent& event) { CloseModal(event.window); }

    void ShowModal(ModalWindow* window) {
        modalWindows_.PushBack(window);

        ImGui::OpenPopup(window->Name());
        window->Init();
    }

    void CloseModal(ModalWindow* window) {
        ImGui::CloseCurrentPopup();
        modalWindows_.EraseIf([&](auto w) { return w == window; });
    }

    void OnError(const ErrorEvent& event) {
        UGINE_ERROR(event.error);

        ShowMessage(MessageWindow::Type::Error, event.error);
    }

    void ShowMessage(MessageWindow::Type type, StringView text) {
        messageWindow_.SetType(type);
        messageWindow_.SetText(text);
        ShowModal(&messageWindow_);
    }

    EditorContext context_;
    ResourceThumbnailCreator thumbnailCreator_;
    Vector<ModalWindow*> modalWindows_;
    MessageWindow messageWindow_;

    bool imGuiDemoVisible_{};
};

} // namespace ugine::ed

int main(int argc, char* argv[]) {
    try {
        ugine::EngineParams params = {
            .appName = "uGine Editor",
            .appVersion = { UGINE_EDITOR_VERSION_MAJOR, UGINE_EDITOR_VERSION_MINOR, UGINE_EDITOR_VERSION_FILE },
            .systems = ugine::Systems::All,
            .fullscreen = false,
            .width = 1600,
            .height = 1200,
#ifdef _DEBUG
            .debugGraphics = true,
#else
            .debugGraphics = false,
#endif
            .imgui = true,
        };

        ugine::Engine engine{ params };
        engine.AddSystem(MakeUnique<ugine::ed::Editor>(engine.GetAllocator(), engine));

        engine.Run();

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
