#include "ShaderTool.h"

#include "../EditorContext.h"
#include "../EditorTool.h"

#include "../platform/FileDialog.h"

#include "ImportShaderWindow.h"
#include "ShaderPreviewWindow.h"

#include <ugine/Memory.h>

namespace ugine::ed {

namespace {
    const Vector<String> SHADER_FILES_FILTER{ "Shader Files", "*.shader", "All files", "*.*" };
}

class ShaderTool : public EditorTool {
public:
    explicit ShaderTool(EditorContext& context)
        : EditorTool{ context }
        , importShader_{ context } {
        auto& importMenu{ context_.MainMenu().Get("Import") };
        importMenu.AddAction(SHADER_RESOURCE_ICON " Shader...", [this] {
            const auto files{ OpenFileDialog(context_.Engine().GetPlatform().GetNativeHandle(), SHADER_FILES_FILTER, false) };
            if (files.Size() == 1) {
                context_.Events().ImportResource<Shader>(files[0]);
            }
        });
    }

    void Render() override {}

private:
    ImportShaderWindow importShader_;
};

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterResourceType<Shader>(SHADER_RESOURCE_ICON, ".ush", ColorRGB{ 1, 1, 0.75f });

        context.RegisterEditorTool(MakeUnique<ShaderTool>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<ShaderPreviewWindow>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed