#pragma once

#include <gfxapi/Handle.h>

namespace ugine::gfxapi {
class CommandList;
}

namespace ugine {

struct RenderContext;
class GraphicsState;

class TonemappingPass {
public:
    explicit TonemappingPass(GraphicsState& state);

    void Render(gfxapi::CommandList& cmd, RenderContext& context);

private:
    gfxapi::RenderPassHandleUnique renderPass_;
    gfxapi::GraphicsPipelineHandleUnique tonemappingPSO_;
};

} // namespace ugine