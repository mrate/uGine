#pragma once

#include <ugine/engine/engine/System.h>
#include <ugine/engine/gfx/GraphicsSystem.h>

#include <gfxapi/Types.h>

#include <memory>

namespace ugine {

class ImGuiSystem final : public System, public Layer {
public:
    explicit ImGuiSystem(Engine& engine);
    ~ImGuiSystem();

    void EarlyUpdate() override;
    void LateUpdate() override;

    bool HandleSystemEvent(const SystemEvent& event) override;

    gfxapi::TextureHandle View() override { return renderTarget_; }
    bool Visible() const override { return visible_; }

private:
    void Init();
    void Resize(u32 width, u32 height);

    gfxapi::TextureHandleUnique fontTexture_;
    i32 fontTextureIndex_{};

    gfxapi::SamplerHandleUnique fontSampler_;
    i32 fontSamplerIndex_{};

    gfxapi::GraphicsPipelineHandleUnique pipeline_;
    gfxapi::SamplerHandleUnique imageSampler_;
    i32 imageSamplerIndex_{};

    gfxapi::Extent2D extent_;
    gfxapi::RenderPassHandleUnique renderpass_;
    gfxapi::TextureHandle renderTarget_;

    bool visible_{};
};

} // namespace ugine