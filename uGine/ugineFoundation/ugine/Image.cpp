#include "Image.h"

#include "Assert.h"
#include "File.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ktx.h>

// TODO: Remove.
#include <vulkan/vulkan.h>

namespace ugine {

struct KtxCleaner {
    KtxCleaner() = default;
    KtxCleaner(ktxTexture* texture)
        : texture{ texture } {}

    ~KtxCleaner() {
        if (texture) {
            ktxTexture_Destroy(texture);
        }
    }

    ktxTexture* texture{};
};

bool Image::FromMemoryEncoded(Span<const u8> memory, Image& image) {
    return FromMemoryEncoded(memory.Data(), memory.Size(), image);
}

bool Image::FromMemoryEncoded(const void* memory, size_t size, Image& image) {
    const auto isKtx{ [&] {
        const auto data{ reinterpret_cast<const u8*>(memory) };
        return size >= 4 && data[0] == 0xab && data[1] == 0x4b && data[2] == 0x54 && data[3] == 0x58;
    }() };

    if (isKtx) {
        KtxCleaner cleaner{};
        ktxResult result{ ktxTexture_CreateFromMemory(
            reinterpret_cast<const ktx_uint8_t*>(memory), size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &cleaner.texture) };
        if (result != KTX_SUCCESS) {
            return false;
        }

        return image.Init(cleaner.texture);
    } else {
        int width{};
        int height{};
        int channels{};

        auto data{ stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(memory), int(size), &width, &height, &channels, 4) };
        if (!data) {
            return false;
        }

        image.width_ = u32(width);
        image.height_ = u32(height);
        image.pixelSize_ = 4;

        image.SetLayers(1);
        image.CopyData(0, Span<const u8>{ data, size_t(width * height * 4) });

        stbi_image_free(data);
        return true;
    }
}

bool Image::FromMemoryDecoded(u32 width, u32 height, Span<const u8> memory, Image& image) {
    image.width_ = u32(width);
    image.height_ = u32(height);
    image.pixelSize_ = 4;

    image.SetLayers(1);
    image.CopyData(0, memory);

    return true;
}

bool Image::FromStream(std::istream& str, Image& image) {
    const auto data{ ReadFileBinary(str) };
    return Image::FromMemoryEncoded(data.Data(), data.Size(), image);
}

void Image::CopyData(u32 layer, Span<const u8> srcData) {
    UGINE_ASSERT(layer < layers_.Size());
    UGINE_ASSERT(srcData.Size() >= width_ * height_ * pixelSize_);
    memcpy(layers_[layer].Data(), srcData.Data(), width_ * height_ * pixelSize_);
}

void Image::SetLayers(u32 layers) {
    const auto prevSize{ layers_.Size() };
    layers_.Reserve(layers);
    for (auto i{ prevSize }; i < layers; ++i) {
        layers_.PushBack(Vector<u8>{ width_ * height_ * pixelSize_, allocator_ });
    }
}

bool Image::Init(ktxTexture* texture) {
    width_ = texture->baseWidth;
    height_ = texture->baseHeight;

    u32 numLayers{ texture->numLayers };
    pixelSize_ = u32(texture->dataSize / width_ / height_ / texture->baseDepth);
    if (texture->isArray) {
        pixelSize_ /= texture->numLayers;
        numLayers = texture->numLayers;
    } else if (texture->isCubemap) {
        UGINE_ASSERT(texture->numFaces == 6);

        pixelSize_ /= texture->numFaces;
        numLayers = texture->numFaces;
        isCubemap_ = true;
    }

    SetLayers(numLayers);

    const auto level{ 0 };

    for (u32 i{}; i < numLayers; ++i) {
        const u32 layer{ isCubemap_ ? 0 : i };
        const u32 face{ isCubemap_ ? i : 0 };

        ktx_size_t offset{};
        KTX_error_code ret{ ktxTexture_GetImageOffset(texture, level, layer, face, &offset) };
        if (ret != KTX_SUCCESS) {
            return false;
        }

        CopyData(i, Span<const u8>{ ktxTexture_GetData(texture) + offset, width_ * height_ * pixelSize_ });
    }

    return true;
}

bool Image::Add(u32 layer, ktxTexture* texture) {
    if (texture->isArray || texture->isCubemap) {
        return false;
    }

    const auto pixelSize{ texture->dataSize / texture->baseWidth / texture->baseHeight / texture->baseDepth };
    if (width_ != texture->baseWidth || height_ != texture->baseHeight || pixelSize_ != pixelSize) {
        return false;
    }

    SetLayers(std::max<u32>(layer + 1, u32(layers_.Size())));

    ktx_size_t offset{};
    KTX_error_code ret{ ktxTexture_GetImageOffset(texture, 0, 0, 0, &offset) };
    if (ret != KTX_SUCCESS) {
        return false;
    }

    CopyData(layer, Span<const u8>{ ktxTexture_GetData(texture) + offset, width_ * height_ * pixelSize_ });

    return true;
}

bool Image::FromFile(const Path& path, Image& image) {
    if (path.Extension() == ".ktx") {
        KtxCleaner cleaner{};
        ktxResult result{ ktxTexture_CreateFromNamedFile(path.Data(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &cleaner.texture) };
        if (result != KTX_SUCCESS) {
            return false;
        }

        return image.Init(cleaner.texture);
    } else {
        int width{};
        int height{};
        int channels{};

        auto data{ stbi_load(path.Data(), &width, &height, &channels, 4) };
        if (!data) {
            return false;
        }

        image.width_ = u32(width);
        image.height_ = u32(height);
        image.pixelSize_ = 4;
        image.SetLayers(1);
        image.CopyData(0, Span<const u8>(data, width * height * 4));

        stbi_image_free(data);

        return true;
    }
}

bool Image::AddLayerFromFile(u32 layer, const Path& path) {
    if (path.Extension() == ".ktx") {
        KtxCleaner cleaner{};
        ktxResult result{ ktxTexture_CreateFromNamedFile(path.Data(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &cleaner.texture) };
        if (result != KTX_SUCCESS) {
            return false;
        }

        return Add(layer, cleaner.texture);
    } else {
        int width{};
        int height{};
        int channels{};

        auto data{ stbi_load(path.Data(), &width, &height, &channels, 4) };
        if (!data) {
            return false;
        }

        if (width_ != u32(width) || height_ != u32(height) || pixelSize_ != 4) {
            return false;
        }

        SetLayers(std::max<u32>(u32(layers_.Size()), layer + 1));
        CopyData(layer, Span<const u8>(data, width * height * 4));

        stbi_image_free(data);

        return true;
    }
}

Span<const u8> Image::GetLayer(u32 layer) const {
    UGINE_ASSERT(layer < layers_.Size());
    return Span{ layers_[layer].Data(), layers_[layer].Size() };
}

Image::Image(IAllocator& allocator)
    : allocator_{ allocator } {
}

Image::Image(u32 width, u32 height, u32 pixelSize, u32 layers, IAllocator& allocator)
    : allocator_{ allocator }
    , width_{ width }
    , height_{ height }
    , pixelSize_{ pixelSize }
    , layers_{ allocator } {
    SetLayers(layers);
}

Image::~Image() {
}

bool Image::Save(const Path& path, bool compress) const {
    UGINE_ASSERT(pixelSize_ == 4);

    ktxTexture2* texture{};

    KTX_error_code result{};

    ktxTextureCreateInfo createInfo{};
    // VK_FORMAT_BC1_RGBA_UNORM_BLOCK
    createInfo.vkFormat = compress ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK : VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.baseWidth = width_;
    createInfo.baseHeight = height_;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2; //
    createInfo.numLevels = 1;     // Mips.
    createInfo.numLayers = ktx_uint32_t(isCubemap_ ? 1 : layers_.Size());
    createInfo.numFaces = ktx_uint32_t(isCubemap_ ? layers_.Size() : 1);
    createInfo.isArray = (!isCubemap_ && layers_.Size() > 1) ? KTX_TRUE : KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
    if (result != KTX_SUCCESS) {
        return false;
    }

    //
    const ktx_uint32_t mipLevel{ 0 };
    const ktx_uint32_t face{ 0 };
    const ktx_uint32_t layerSize{ width_ * height_ * pixelSize_ };

    for (u32 i{}; i < layers_.Size(); ++i) {
        const u32 layer{ isCubemap_ ? 0 : i };
        const u32 face{ isCubemap_ ? i : 0 };

        result = ktxTexture_SetImageFromMemory(ktxTexture(texture), mipLevel, layer, face, reinterpret_cast<const ktx_uint8_t*>(GetLayer(i).Data()), layerSize);

        if (result != KTX_SUCCESS) {
            ktxTexture_Destroy(ktxTexture(texture));
            return false;
        }
    }

    result = ktxTexture_WriteToNamedFile(ktxTexture(texture), path.Data());
    ktxTexture_Destroy(ktxTexture(texture));

    return result == KTX_SUCCESS;
}

} // namespace ugine