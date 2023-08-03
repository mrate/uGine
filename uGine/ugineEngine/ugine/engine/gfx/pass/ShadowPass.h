#pragma once

#include "GeometryPass.h"

namespace ugine {

class ShadowPass final : public GeometryPass {
public:
    explicit ShadowPass(GraphicsState& state);

    void RenderShadows(gfxapi::CommandList& cmd, RenderContext& context);

private:
    gfxapi::RenderPassHandleUnique renderPass_;
};

} // namespace ugine