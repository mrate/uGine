#pragma once

#include <ugine/engine/gfx/Material.h>

namespace ugine::gfxapi {
class CommandList;
}

namespace ugine {

struct RenderContext;

class GeometryPass {
public:
    virtual ~GeometryPass() = default;

protected:
    void RenderGeometry(gfxapi::CommandList& cmd, RenderContext& context, bool depth, bool transparent);
};

} // namespace ugine