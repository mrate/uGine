#pragma once

#include <ugine/EventEmittor.h>
#include <ugine/engine/core/Resource.h>

#include "../EditorContext.h"

#include "ImGuiEx.h"

namespace ImGuiEx {

enum class SelectResourceFlags : u8 {
    LocateButton = UGINE_BIT(0),
    EditButton = UGINE_BIT(1),
    ResourceLoading = UGINE_BIT(2),
    ResourceError = UGINE_BIT(3),
    AllowNull = UGINE_BIT(4),

    Default = LocateButton | EditButton | AllowNull,
};

UGINE_FLAGS(SelectResourceFlags, u8);

bool SelectResource(
    ugine::ed::EditorContext& context, const ugine::ResourceType& type, ugine::ResourceID& selected, SelectResourceFlags flags = SelectResourceFlags::Default);

template <typename T>
bool SelectResource(ugine::ed::EditorContext& context, ugine::ResourceHandle<T>& handle, SelectResourceFlags flags = SelectResourceFlags::Default) {
    ugine::ResourceID id{};
    if (handle) {
        id = handle->Id();
        switch (handle->State()) {
        case ugine::ResourceState::Loading: flags |= SelectResourceFlags::ResourceLoading; break;
        case ugine::ResourceState::Failed: flags |= SelectResourceFlags::ResourceError; break;
        }
    }

    const auto resource{ SelectResource(context, T::TYPE, id, flags) };
    if (resource) {
        handle = context.Engine().GetResources().Get<T>(id);
    }
    return resource;
}

} // namespace ImGuiEx
