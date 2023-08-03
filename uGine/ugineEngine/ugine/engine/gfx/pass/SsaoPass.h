#pragma once

#include <gfxapi/Handle.h>
#include <gfxapi/Types.h>

namespace ugine::gfxapi {
class CommandList;
}

namespace ugine {

struct RenderContext;
class GraphicsState;

class SsaoPass {
public:
    explicit SsaoPass(GraphicsState& state);

    void Render(gfxapi::CommandList& cmd, RenderContext& context);

private:
    inline static gfxapi::Format FORMAT{ gfxapi::Format::R8_Unorm };

    gfxapi::RenderPassHandleUnique renderPass_;
    gfxapi::ComputePipelineHandleUnique blurCSO_;
    gfxapi::GraphicsPipelineHandleUnique ssaoPSO_;
};

} // namespace ugine