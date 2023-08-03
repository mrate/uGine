#pragma once

#include <gfxapi/Types.h>

#include <ugine/engine/assets/MaterialDesignIcons.h>

#include <imgui.h>

namespace ugine {

enum class ImFlags : u32 {
    None = 0,
    Depth = UGINE_BIT(1),
};

UGINE_FLAGS(ImFlags, u32);

ImTextureID ImGuiTextureHandle(gfxapi::TextureHandle texture, gfxapi::Device& device, ImFlags flags = ImFlags::None);

inline ImTextureID ImGuiTexture(i32 textureBindlessIndex, ImFlags flags = ImFlags::None) {
    return reinterpret_cast<ImTextureID>((u64(flags) << 32) | textureBindlessIndex);
}

} // namespace ugine