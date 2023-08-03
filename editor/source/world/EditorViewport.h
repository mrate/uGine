#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include "../widgets/TransformationWidget.h"

#include <gfxapi/CommandList.h>

#include <ugine/EventEmittor.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/math/Raycast.h>
#include <ugine/engine/script/LuaScript.h>
#include <ugine/engine/utils/CameraController.h>
#include <ugine/engine/world/World.h>

#include <imgui_internal.h>

namespace ugine {
class GraphicsState;
class InputState;
} // namespace ugine

namespace ugine::ed {

class EditorContext;

class EditorViewport final : public EventEmittor, public EditorTool {
public:
    template <typename T> struct ResourceDroppedInViewport {
        Transformation transformation;
        ResourceHandle<T> resource;
        GameObject activeGO;
    };

    explicit EditorViewport(EditorContext& context);

    void SetWorld(World* world, GameObject cameraGO);

    void PlaceModel(const Transformation& transformation, ResourceHandle<Model> model);
    void PlaceScript(const Transformation& transformation, ResourceHandle<LuaScript> script);

    void Render() override;

private:
    enum class Operation {
        Select,
        Transform,
    };

    static inline constexpr glm::vec3 OBJECT_PLACEMENT_OFFSET{ 0, 0, -3 };

    void OnWorldChanged(const WorldChangedEvent& event);

    bool UpdateContent();
    void RenderContent();
    void RenderToolbar(const ImVec2& position);

    void Resize(uint32_t width, uint32_t height);

    bool HandleInput(const ImRect& sceneRect);
    void HandleDragAndDrop();

    void InitCameraGO();

    ImGuiEx::TransformationWidget transformation_;

    GameObject cameraGO_;
    GraphicsState* gfxState_{};
    InputState* inputState_{};

    glm::mat4 viewProjection_;

    Operation operation_{ Operation::Select };

    bool localTransformation_{};
    bool isTransforming_{};

    uint32_t width_{};
    uint32_t height_{};

    CameraController controller_;

    bool snapEnabled_{};
    float snapValue_{ 1.0f };

    Ray ray_;
    std::optional<WorldHit> hit_{};
};

} // namespace ugine::ed