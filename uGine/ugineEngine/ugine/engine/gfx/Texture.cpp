#include "Texture.h"

#include <ugine/Image.h>
#include <ugine/Profile.h>

#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/math/Math.h>

namespace ugine {

Texture::Texture(ResourceManager& resourceManager, const ResourceID& id)
    : Resource{ resourceManager, TYPE, id }
    , bindlessIndex_{ resourceManager.GetAllocator() } {
}

bool Texture::HandleLoad(Span<const u8> data) {
    PROFILE_EVENT();

    Image image{ Manager().GetAllocator() };
    if (!Image::FromMemoryEncoded(data, image)) {
        return false;
    }
    using namespace gfxapi;

    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    Vector<SubresourceData> initialData{ image.Layers(), Manager().GetAllocator() };
    for (u32 layer{}; layer < image.Layers(); ++layer) {
        initialData[layer] = SubresourceData{
            .data = image.GetLayer(layer).Data(),
            .size = u64(image.Width() * image.Height() * image.PixelSize()),
            .pitch = u64(image.Width() * image.PixelSize()),
            .slicePitch = u64(image.Width() * image.Height() * image.PixelSize()),
        };
    }

    layers_ = image.Layers();

    // TODO: Exceptions.
    // TODO: Depth/stencil support?
    try {
        TextureMiscFlags miscFlags{};
        if (image.IsCubemap()) {
            miscFlags |= TextureMiscFlags::Cube;
        }

        texture_ = state->device.CreateTexture(
            TextureDesc{
                .name = "TextureResource", // TODO:
                .extent = Extent2D{ image.Width(), image.Height() },
                .arrayLayers = image.Layers(),
                .format = Format::R8G8B8A8_Unorm, // TODO: Support other formats.
                .usage = TextureUsageFlags::Sampled,
                .misc = miscFlags,
                .mipLevels = CalculateMipLevels(image.Width(), image.Height()),
                .generateMips = true,
            },
            TextureLayout::ReadOnly, initialData);

        bindlessIndex_.Resize(layers_);
        bindlessIndex_[0] = state->device.GetTextureBindlessIndex(texture_, TextureAspectFlags::Color);
        for (u32 i{ 1 }; i < layers_; ++i) {
            bindlessIndex_[i] = gfxapi::BindlessInvalid;
        }

        return true;
    } catch (const std::exception& ex) {
        UGINE_ERROR("Failed to load texture: {}", ex.what());
        return false;
    }
}

bool Texture::HandleUnload() {
    PROFILE_EVENT();

    auto state{ Manager().GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);

    if (texture_) {
        state->device.DestroyTexture(texture_);
    }
    texture_ = {};
    layers_ = 0;
    bindlessIndex_.Clear();

    return true;
}

} // namespace ugine