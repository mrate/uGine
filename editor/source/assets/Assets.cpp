#include "../EditorContext.h"
#include "../EditorTool.h"

#include "AssetRegistry.h"
#include "AssetsBrowser.h"
#include "AssetsMonitor.h"

#include <ugine/Memory.h>

namespace ugine::ed {

namespace {
    EditorToolset::Register _{ [](EditorContext& context) {
        context.RegisterEditorTool(MakeUnique<AssetsBrowser>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<AssetsMonitor>(context.Allocator(), context));
        context.RegisterEditorTool(MakeUnique<AssetRegistry>(context.Allocator(), context));
    } };
} // namespace

} // namespace ugine::ed