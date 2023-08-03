#include "../EditorContext.h"
#include "../EditorTool.h"

#include "AnimationEditorWindow.h"

#include <ugine/Memory.h>

namespace ugine::ed {

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterResourceType<Animation>(ICON_FA_FILM, ".uanim", ColorRGB{ 0.75f, 1, 1 });

        context.RegisterEditorTool(MakeUnique<AnimationEditorWindow>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed