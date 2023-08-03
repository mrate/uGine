#pragma once

#include "../EditorTool.h"

#include <ugine/engine/gfx/ImGui.h>

namespace ugine {
class GraphicsState;
}

namespace ugine::ed {

class EditorContext;

class GameViewport final : public EditorTool {
public:
    explicit GameViewport(EditorContext& context);

    void Render() override;

private:
    GraphicsState* gfxState_{};
    ImVec2 sceneSize_{};
};

} // namespace ugine::ed