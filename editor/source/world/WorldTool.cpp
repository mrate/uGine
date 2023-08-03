#include "WorldTool.h"
#include "../EditorContext.h"
#include "../EditorTool.h"

#include "CameraPreview.h"
#include "GameObjectEditor.h"
#include "GameViewport.h"
#include "WorldEditor.h"
#include "EditorViewport.h"

#include <ugine/Memory.h>

namespace ugine::ed {

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterResourceType<WorldDescriptor>(WORLD_RESOURCE_ICON, ".uworld", ColorRGB{ 1, 0.75f, 1 });

        context.RegisterEditorTool(MakeUnique<GameObjectEditor>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<WorldEditor>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<EditorViewport>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<GameViewport>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<CameraPreview>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed