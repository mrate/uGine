#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include "../preview/WorldPreviewRenderer.h"

#include <gfxapi/Types.h>
#include <ugine/engine/gfx/ImGui.h>
#include <ugine/engine/gfx/Model.h>
#include <ugine/engine/world/World.h>

namespace ugine::ed {

class EditorContext;

class ModelEditorWindow : public EditorTool {
public:
    explicit ModelEditorWindow(EditorContext& context);

    void SetModel(ResourceHandle<Model> model);
    void ResetCamera();

    void Render() override;

private:
    enum class Controlling {
        None,
        Rotation,
        Translation,
    };

    void OnOpenResource(const OpenResourceEvent<Model>& resource);
    void OnOpenTypelessResource(const OpenTypelessResourceEvent& event);

    void RenderContent();

    void RenderSocketProperties();

    void RenderPreview();
    void RenderBoneTree(int& id, uint32_t i);
    void RenderModelProperties();

    void RenderMaterialsTab();
    void RenderPopups();
    void RenderPreviewTab();

    void UpdateModel();
    void UpdateSocket();

    void UpdateCameraControl(const ImVec2& position);

    void Resize(uint32_t width, uint32_t height);
    void UpdateCamera();
    glm::vec3 GetCameraPosition() const;

    WorldPreviewRenderer preview_;
    std::vector<Transformation> nodeTransform_;

    ResourceHandle<Model> model_;
    ResourceHandle<Animation> previewAnimation_;

    bool isSkinned_{};
    bool animationPlay_{};

    int popup_{};
    StringID selectedSocket_{};

    bool testSocketEnabled_{};
    StringID testSocketName_{};
    GameObject testSocketGO_;

    uint32_t width_{};
    uint32_t height_{};

    Controlling controlling_{ Controlling::None };

    bool modelReady_{};
    glm::vec3 cameraOffset_{};
    float cameraDistance_{ 1.0f };
    float theta_{};
    float phi_{};

    ImVec2 delta_{};
};

} // namespace ugine::ed