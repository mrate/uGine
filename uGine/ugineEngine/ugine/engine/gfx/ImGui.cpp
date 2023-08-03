#include "ImGui.h"

#include <gfxapi/Device.h>

namespace ugine {

ImTextureID ImGuiTextureHandle(gfxapi::TextureHandle texture, gfxapi::Device& device, ImFlags flags) {
    return ImGuiTexture(device.GetTextureBindlessIndex(texture), flags);
}

} // namespace ugine