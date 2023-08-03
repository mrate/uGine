#pragma once

#include "../preview/WorldPreviewRenderer.h"

#include <gfxapi/Types.h>
#include <ugine/engine/gfx/Material.h>
#include <ugine/engine/world/Transformation.h>

#include <imgui.h>

namespace ugine::ed {

class WorldPreview;
class EditorContext;

class MaterialPreviewRenderer {
public:
    explicit MaterialPreviewRenderer(EditorContext& context)
        : context_{ context }
        , preview_{ context, false } {}

    void Resize(u32 width, u32 height);
    std::pair<u32, u32> Size() const { return std::make_pair(width_, height_); }

    void SetMaterial(ResourceHandle<Material> material);
    void SetPreviewModel(WorldPreviewRenderer::MaterialPreview model) { preview_.SetMaterialPreviewModel(model); }
    WorldPreviewRenderer::MaterialPreview PreviewModel() const { return preview_.MaterialPreviewModel(); }

    void Render();
    void SetRendering(bool rendering);

private:
    void UpdateCamera();

    EditorContext& context_;
    WorldPreviewRenderer preview_;

    u32 width_{};
    u32 height_{};

    bool controlling_{};
    ImVec2 delta_{};

    float cameraDistance_{ 1.0f };
    float theta_{};
    float phi_{};
};

} // namespace ugine::ed