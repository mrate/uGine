#pragma once

#include "GeometryPass.h"

namespace ugine {

class DepthPrePass : GeometryPass {
public:
    explicit DepthPrePass(GraphicsState& state);

    void RenderDepth(gfxapi::CommandList& cmd, RenderContext& context);
};

} // namespace ugine