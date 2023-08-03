#include "EditorViewport.h"
#include "Actions.h"

#include "../EditorContext.h"
#include "../EditorScene.h"
#include "../widgets/ImGuiScope.h"

#include <ugine/Profile.h>
#include <ugine/engine/engine/CVars.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/gfx/GraphicsScene.h>
#include <ugine/engine/input/InputState.h>
#include <ugine/engine/script/Component.h>
#include <ugine/engine/world/World.h>
#include <ugine/engine/world/WorldManager.h>

#include <ImGuizmo.h>

#include <glm/gtx/transform.hpp>

using namespace ImGuiEx;

namespace ugine::ed {

namespace {
    auto& DebugRay{ CVars::Register("Debug ray", "Enable raycast debugging", "editor", CVar::Type::Bool, false) };
    auto& DebugHit{ CVars::Register("Debug ray hit", "Enable raycast hit debugging", "editor", CVar::Type::Bool, false) };

    const char* SNAP_SETTINGS{ "editor-snap" };
} // namespace

EditorViewport::EditorViewport(EditorContext& context)
    : EditorTool{ context }
    , gfxState_{ context_.Engine().GetState<GraphicsState>() }
    , inputState_{ context_.Engine().GetState<InputState>() } {
    UGINE_ASSERT(gfxState_);
    UGINE_ASSERT(inputState_);

    context_.Events().Connect<WorldChangedEvent, &EditorViewport::OnWorldChanged>(this);
    snapValue_ = context_.Settings().Get(SNAP_SETTINGS, 1.0f);
}

void EditorViewport::SetWorld(World* world, GameObject cameraGO) {
    cameraGO_ = cameraGO;
    InitCameraGO();
}

void EditorViewport::Render() {
    ImScope::StyleVar padding{ ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 } };
    if (ImGui::Begin(ICON_FA_CAMERA " Viewport", nullptr, ImGuiWindowFlags_NoCollapse)) {
        if (!context_.ActiveWorld()) {
            ImGui::Text("No world, no fun :-(");
        } else if (UpdateContent()) {
            RenderContent();
        }
    }

    ImGui::End();
}

void EditorViewport::OnWorldChanged(const WorldChangedEvent& event) {
    for (auto [entt, tag] : event.world->Registry().view<TagComponent>().each()) {
        if (tag.flags & TagComponent::Flags::Editor) {
            // TODO: multiple viewports:
            //int viewportIndex{};
            //std::string name{ tag.name };
            //if (name.starts_with("__viewport#")) {
            //    viewportIndex = std::atoi(name.substr(10).c_str());
            //}

            SetWorld(event.world, event.world->Get(entt));
            return;
        }
    }

    auto cameraGO{ event.world->CreateObject("__editorViewportCamera#0") };
    cameraGO.SetFlags(cameraGO.Flags() | TagComponent::Flags::Editor);
    SetWorld(event.world, cameraGO);
}

bool EditorViewport::UpdateContent() {
    const auto viewportPosition{ ImGui::GetCursorPos() };
    const auto viewportSize{ ImGui::GetContentRegionAvail() };

    if (!cameraGO_ || viewportSize.x < 0 || viewportSize.y < 0) {
        return false;
    }

    if (viewportSize.x != width_ || viewportSize.y != height_) {
        Resize(u32(viewportSize.x), u32(viewportSize.y));
    }

    return true;
}

void EditorViewport::RenderContent() {
    const auto position{ ImGui::GetCursorPos() };

    const auto viewportSize{ ImGui::GetContentRegionAvail() };
    const ImVec2 sceneSize{ viewportSize.x, viewportSize.y };
    const ImRect sceneRect{ position.x, position.y, sceneSize.x, sceneSize.y };

    ImGuiWindowFlags flags{};
    flags |= ImGuiWindowFlags_NoDocking;
    flags |= ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoScrollbar;
    flags |= ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetCursorPos(position);

    ImGui::BeginChild("Scene", sceneSize, flags);

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(sceneRect.Min.x, sceneRect.Min.y, sceneRect.Max.x, sceneRect.Max.y);
    ImGuizmo::SetID(1);

    const auto imageScreenPosition{ ImGui::GetCursorScreenPos() };
    const auto imagePosition{ ImGui::GetCursorPos() };

    // Camera control.
    auto& selectedGO{ context_.SelectedGO() };

    const auto userInput{ HandleInput(sceneRect) };
    const auto gizmoManipulate{ selectedGO.IsValid() && transformation_.WantCapture() };

    // Viewport + editor layout.
    auto world{ context_.ActiveWorld() };
    auto gfxScene{ world->GetScene<GraphicsScene>() };
    auto editorScene{ world->GetScene<EditorScene>() };

    ImGui::Image(ImGuiTextureHandle(gfxScene->GetCameraRtv(cameraGO_), gfxState_->device), sceneSize);
    ImGui::SetCursorPos(imagePosition);
    ImGui::Image(ImGuiTextureHandle(editorScene->GetCameraRtv(cameraGO_), gfxState_->device), sceneSize);

    const auto pick{ !userInput && !gizmoManipulate && ImGui::IsItemClicked(0) };

    // Drag&drop
    HandleDragAndDrop();

    const auto& camera{ cameraGO_.Component<CameraComponent>() };
    const auto projectionMatrix{ ProjectionMatrix(camera) };

    bool transforming{ transformation_.IsActive() };
    if (operation_ == Operation::Transform && !userInput && selectedGO.IsValid() && cameraGO_.IsValid()) {
        glm::mat4 deltaTransform{};

        auto objectTransformation{ transformation_.IsLocal() ? selectedGO.LocalTransformation() : selectedGO.GlobalTransformation() };
        if (transformation_.Render(deltaTransform, projectionMatrix, cameraGO_.GlobalTransformation(), objectTransformation,
                snapEnabled_ ? std::make_optional<glm::vec3>(snapValue_) : std::nullopt)) {
            if (!isTransforming_) {
                if (ImGui::GetIO().KeyCtrl) {
                    DuplicateGO(context_, context_.SelectedGO());
                }
            }

            Transformation t{ localTransformation_ ? selectedGO.LocalTransformation() : selectedGO.GlobalTransformation() };

            switch (transformation_.GetOperation()) {
            case TransformationWidget::Operation::Translate: t.Translate(deltaTransform); break;
            case TransformationWidget::Operation::Rotate: t.Rotate(deltaTransform); break;
            case TransformationWidget::Operation::Scale:
            case TransformationWidget::Operation::Bounds: t.Scale(deltaTransform); break;
            }

            if (localTransformation_) {
                selectedGO.SetLocalTransformation(t);
            } else {
                selectedGO.SetGlobalTransformation(t);
            }
        }
    }

    isTransforming_ = transforming;

    if (operation_ == Operation::Select && pick) {
        const auto mousePos{ ImGui::GetMousePos() };
        const auto x{ mousePos.x - imageScreenPosition.x };
        const auto y{ mousePos.y - imageScreenPosition.y };
        const auto viewMatrix{ glm::inverse(cameraGO_.GlobalTransformation().Matrix()) };

        ray_ = RayFromCamera(projectionMatrix, viewMatrix, x, y, f32(camera.width), f32(camera.height));
        auto pick{ world->RayCast(ray_) };
        if (pick.hit) {
            hit_ = pick;
            context_.SelectGO(world->Get(pick.object));
        } else {
            hit_ = std::nullopt;
            context_.DeselectGO();
        }
    }

    ImGui::EndChild();

    RenderToolbar(ImVec2{ position.x + 6, position.y + 6 });
}

void EditorViewport::RenderToolbar(const ImVec2& position) {
    const auto& padding{ ImGui::GetStyle().FramePadding };
    const auto& spacing{ ImGui::GetStyle().ItemSpacing };

    const ImVec2 toolbarButtonSize{ 20, 20 };
    auto toolbarPosition{ position };

    auto toolbarIcon = [&](const char* label, bool active, auto action) {
        const auto color{ ImGui::GetStyle().Colors[active ? ImGuiCol_Text : ImGuiCol_TextDisabled] };
        ImScope::Color scopeColor{ ImGuiCol_Text, color };

        const auto fs{ ImGui::GetFontSize() };
        const auto pos{ ImGui::GetCursorPos() };

        ImGui::SetCursorPos(ImVec2{ pos.x + (toolbarButtonSize.x - fs) / 2, pos.y + (toolbarButtonSize.y - fs) / 2 });

        ImGui::TextUnformatted(label);
        if (ImGui::IsItemClicked(0)) {
            action();
        }

        ImGui::SetCursorPos(ImVec2{ pos.x + toolbarButtonSize.x + spacing.x, pos.y });
    };

    ImGui::SetCursorPos(toolbarPosition);

    const ImVec2 toolbarSize{ 4 * toolbarButtonSize.x + 2 * spacing.x + 2 * padding.x, toolbarButtonSize.y + 2 * padding.y };
    if (ImGuiEx::BeginToolbar(1, toolbarSize)) {
        auto transformButton = [&](const char* label, TransformationWidget::Operation operation) {
            toolbarIcon(label, operation_ == Operation::Transform && transformation_.GetOperation() == operation, [&] {
                operation_ = Operation::Transform;
                transformation_.SetOperation(operation);
            });
        };

        toolbarIcon(ICON_FA_CURSOR_DEFAULT, operation_ == Operation::Select, [&] { operation_ = Operation::Select; });
        transformButton(ICON_FA_AXIS_ARROW, TransformationWidget::Operation::Translate);
        transformButton(ICON_FA_ROTATE_ORBIT, TransformationWidget::Operation::Rotate);
        transformButton(ICON_FA_RESIZE, TransformationWidget::Operation::Scale);
    }
    ImGuiEx::EndToolbar();

    toolbarPosition.x += toolbarSize.x + 2 * spacing.x;
    ImGui::SetCursorPos(toolbarPosition);

    if (ImGuiEx::BeginToolbar(2, ImVec2{ 100, toolbarSize.y })) {
        const auto isPerspective{ cameraGO_.Component<CameraComponent>().projection == Camera::ProjectionType::Perspective };
        toolbarIcon(isPerspective ? ICON_FA_VIDEO : ICON_FA_CAMERA, true, [&] {
            cameraGO_.Patch<CameraComponent>(
                [&](auto& c) { c.projection = isPerspective ? Camera::ProjectionType::Ortho : Camera::ProjectionType::Perspective; });
        });

        toolbarIcon(ICON_FA_GRID_LARGE, snapEnabled_, [&] { snapEnabled_ = !snapEnabled_; });

        if (ImGui::InputFloat("##snap", &snapValue_)) {
            context_.Settings().Set(SNAP_SETTINGS, snapValue_);
        }
    }
    ImGuiEx::EndToolbar();

    toolbarPosition.x += 100 + 2 * spacing.x;
    ImGui::SetCursorPos(toolbarPosition);

    if (ImGuiEx::BeginToolbar(3, ImVec2{ 75, toolbarSize.y })) {
        {
            ImScope::Color color{ ImGuiCol_Text, ImVec4{ 0, 1, 0, 1 } };
            toolbarIcon(ICON_FA_PLAY, context_.IsPlaying(), [&] { context_.Events().StartWorldSimulation(); });
        }

        {
            ImScope::Color color{ ImGuiCol_Text, ImVec4{ 1, 1, 0, 1 } };
            toolbarIcon(ICON_FA_PAUSE, context_.IsPaused(), [&] { context_.Events().PauseWorldSimulation(!context_.IsPaused()); });
        }

        {
            ImScope::Color color{ ImGuiCol_Text, ImVec4{ 1, 0, 0, 1 } };            
            toolbarIcon(ICON_FA_STOP, context_.IsPlaying(), [&] { context_.Events().StopWorldSimulation(); });
        }
    }
    ImGuiEx::EndToolbar();
}

bool EditorViewport::HandleInput(const ImRect& sceneRect) {
    ImGuiIO& io{ ImGui::GetIO() };

    if (ImGui::IsWindowFocused()) {
        if (WorldShortcuts(context_)) {
            return false;
        }
    }

    if (!(ImGui::IsWindowHovered() && sceneRect.Contains(io.MousePos) && ImGui::IsMousePosValid())) {
        return false;
    }

    if (!(io.WantCaptureMouse && io.MouseDown[1])) {
        return false;
    }

    inputState_->AddInput(INPUT_MOUSE_DRAG_X, io.MouseDelta.x);
    inputState_->AddInput(INPUT_MOUSE_DRAG_Y, io.MouseDelta.y);

    auto transformation{ cameraGO_.GlobalTransformation() };
    controller_.Update(context_.Engine(), transformation);
    cameraGO_.SetGlobalTransformation(transformation);

    return true;
}

void EditorViewport::HandleDragAndDrop() {
    auto trans{ cameraGO_.GlobalTransformation() };
    trans.position += trans.rotation * OBJECT_PLACEMENT_OFFSET;

    const auto ctrlPressed{ ImGui::GetIO().KeyCtrl };

    ResourceHandle<WorldDescriptor> world;
    if (context_.DragAndDrop().DropResource<WorldDescriptor>(world)) {
        const auto clearWorld{ !ctrlPressed };
        context_.Events().OpenWorld(world, clearWorld);
    }

    context_.DragAndDrop().DropResourceMulti([&](const auto& id, const auto& type) {
        if (type == Model::TYPE) {
            auto model{ context_.Engine().GetResources().Get<Model>(id) };
            PlaceModel(trans, model);
        } else if (type == LuaScript::TYPE) {
            auto script{ context_.Engine().GetResources().Get<LuaScript>(id) };
            PlaceScript(trans, script);
        }
    });
}

void EditorViewport::InitCameraGO() {
    if (!cameraGO_ || width_ == 0 || height_ == 0) {
        return;
    }

    // TODO: Camera is created this frame but render data will not be updated until next frame :-/
    auto editorScene{ context_.ActiveWorld()->GetScene<EditorScene>() };
    editorScene->AddEditorData(cameraGO_);

    cameraGO_.GetOrCreateComponent<CameraComponent>();
    cameraGO_.Patch<CameraComponent>([&](auto& camera) {
        camera.vFovDeg = 60.0f;
        camera.width = width_;
        camera.height = height_;
        camera.zNear = 0.001f;
        camera.zFar = 10000.0f;
        camera.projection = Camera::ProjectionType::Perspective;
        camera.clearColor = ColorRGBA{ 0.16f, 0.23f, 0.31f, 1 };
    });

    controller_.Init(cameraGO_.GlobalTransformation());
}

void EditorViewport::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    if (!cameraGO_ || width_ == 0 || height_ == 0) {
        return;
    }

    InitCameraGO();
}

void EditorViewport::PlaceModel(const Transformation& transformation, ResourceHandle<Model> model) {
    GameObject go{ context_.ActiveWorld()->CreateObject(context_.Engine().GetResources().ResourceName(model->Id()).Data()) };
    go.CreateComponent<MeshComponent>(model);

    go.MoveTo(transformation.position);
    context_.SelectGO(go);
}

void EditorViewport::PlaceScript(const Transformation& transformation, ResourceHandle<LuaScript> script) {
    GameObject go{ context_.ActiveWorld()->CreateObject(context_.Engine().GetResources().ResourceName(script->Id()).Data()) };
    go.CreateComponent<LuaScriptComponent>(script);

    go.MoveTo(transformation.position);
    context_.SelectGO(go);
}

} // namespace ugine::ed