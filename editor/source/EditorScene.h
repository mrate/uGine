#pragma once

#include <ugine/engine/gfx/helpers/DebugRenderer.h>
#include <ugine/engine/world/World.h>

namespace ugine {
class GraphicsState;

namespace gfxapi {
    class CommandList;
}
} // namespace ugine

namespace ugine::ed {

class EditorContext;

struct EditorViewportRenderData;

class EditorScene final : public WorldScene {
public:
    inline static const auto NAME{ "EditorScene"_hs };

    EditorScene(Engine& engine, World& world, EditorContext& context, IAllocator& allocator);
    ~EditorScene();

    gfxapi::TextureHandle GetCameraRtv(const GameObject& go) const;

    void AddEditorData(GameObject& go);
    void Render(gfxapi::CommandList& cmd);

    // WorldScene:
    //WorldHit RayCast(const Ray& ray) const override;
    StringID Name() const override { return NAME; }
    void Update() override;

private:
    void RenderOutline(gfxapi::CommandList& cmd, const EditorViewportRenderData& renderData);
    void RenderSelected(gfxapi::CommandList& cmd, const EditorViewportRenderData& renderData);

    EditorContext& context_;
    GraphicsState* gfxState_{};

    gfxapi::RenderPassHandleUnique renderPass_;
    gfxapi::RenderPassHandleUnique outlineRenderPass_;
    gfxapi::GraphicsPipelineHandleUnique outlinePSO_;

    DebugRenderer debugRenderer_; // TODO:
};

} // namespace ugine::ed