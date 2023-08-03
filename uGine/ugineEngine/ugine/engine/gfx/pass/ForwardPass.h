#pragma once

#include "GeometryPass.h"

namespace ugine {

class ForwardPass final : public GeometryPass {
public:
    void Render(gfxapi::CommandList& cmd, RenderContext& context);
};

} // namespace ugine