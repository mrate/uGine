#pragma once

#include <ugine/engine/core/Resource.h>
#include <ugine/engine/core/ResourceManager.h>

#include <gfxapi/Handle.h>
#include <gfxapi/Types.h>

namespace ugine {

class Image;

class Texture final : public Resource {
public:
    inline static const ResourceType TYPE{ "Texture" };

    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    Texture(ResourceManager& resourceManager, const ResourceID& id);

    gfxapi::TextureHandle Get() const { return texture_; }
    u32 GetBindlessIndex(u32 layer = 0) const { return layer < bindlessIndex_.Size() ? bindlessIndex_[layer] : gfxapi::BindlessInvalid; }
    u32 Layers() const { return layers_; }

protected:
    bool HandleLoad(Span<const u8> data) override;
    bool HandleUnload() override;

private:
    gfxapi::TextureHandle texture_;
    u32 layers_{};
    Vector<i32> bindlessIndex_;
};

} // namespace ugine