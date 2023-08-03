#include "TextureTool.h"

#include "../EditorContext.h"
#include "../EditorTool.h"

#include "ImportTextureWindow.h"
#include "PreviewTextureWindow.h"

#include <ugine/Memory.h>

namespace ugine::ed {

class TextureTool : public EditorTool {
public:
    explicit TextureTool(EditorContext& context)
        : EditorTool{ context }
        , importTexture_{ context } {
        auto& importMenu{ context_.MainMenu().Get("Import") };
        importMenu.AddAction(TEXTURE_RESOURCE_ICON " Texture...", [this] { context_.Events().ShowModal(&importTexture_); });
    }

    void Render() override {}

private:
    ImportTextureWindow importTexture_;
};

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterResourceType<Texture>(TEXTURE_RESOURCE_ICON, ".ktx", ColorRGB{ 0.75f, 0.75f, 1 });

        context.RegisterEditorTool(MakeUnique<TextureTool>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<PreviewTextureWindow>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed