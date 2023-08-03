#pragma once

#include "../EditorEvents.h"
#include "../EditorTool.h"

#include <ugine/engine/gfx/Shader.h>

namespace ugine::ed {

class ShaderPreviewWindow final : public EditorTool {
public:
    explicit ShaderPreviewWindow(EditorContext& context);

    void SetShader(ResourceHandle<Shader> shader);

    void Render() override;

private:
    void OnOpenResource(const OpenResourceEvent<Shader>& event);
    void OnOpenTypelessResource(const OpenTypelessResourceEvent& event);

    void PopulateShader();

    ResourceHandle<Shader> shader_;
};

} // namespace ugine::ed