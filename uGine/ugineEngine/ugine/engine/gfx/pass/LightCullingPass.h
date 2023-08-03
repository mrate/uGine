#pragma once

#include <gfxapi/Types.h>

namespace ugine::gfxapi {
class CommandList;
}

namespace ugine {

class GraphicsState;
struct RenderContext;

class LightCullingPass {
public:
    explicit LightCullingPass(GraphicsState& state);

    void CullLights(gfxapi::CommandList& cmd, RenderContext& context);

    void DebugLights(gfxapi::CommandList& cmd, RenderContext& context, bool transparent = false);

private:
    gfxapi::ComputePipelineHandleUnique lightCullCSO_;
    gfxapi::GraphicsPipelineHandleUnique lightCullDebugPSO_;
};

} // namespace ugine