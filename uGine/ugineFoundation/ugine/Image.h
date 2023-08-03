#pragma once

#include <ugine/Memory.h>
#include <ugine/Span.h>
#include <ugine/Ugine.h>
#include <ugine/Utils.h>
#include <ugine/Vector.h>
#include <ugine/Path.h>

struct ktxTexture;

namespace ugine {

class Image {
public:
    enum Faces {
        XPos = 0,
        XNeg,
        YPos,
        YNeg,
        ZPos,
        ZNeg,
    };

    static bool FromMemoryEncoded(const void* memory, size_t size, Image& image);
    static bool FromMemoryEncoded(Span<const u8> memory, Image& image);
    static bool FromMemoryDecoded(u32 width, u32 height, Span<const u8> memory, Image& image);
    static bool FromFile(const Path& path, Image& image);
    static bool FromStream(std::istream& str, Image& image);

    Image(IAllocator& allocator = IAllocator::Default());
    Image(u32 width, u32 height, u32 pixelSize, u32 layers, IAllocator& allocator = IAllocator::Default());
    ~Image();

    Image(const Image& other) noexcept = default;
    Image& operator=(const Image& other) noexcept = default;

    Image(Image&& other) noexcept = default;
    Image& operator=(Image&& other) noexcept = default;

    u32 Width() const { return width_; }
    u32 Height() const { return height_; }
    u32 Layers() const { return u32(layers_.Size()); }
    u32 PixelSize() const { return pixelSize_; }

    void SetLayers(u32 layers);
    void CopyData(u32 layer, Span<const u8> srcData);
    bool AddLayerFromFile(u32 layer, const Path& path);

    Span<const u8> GetLayer(u32 layer = 0) const;

    bool Save(const Path& path, bool compress = false) const;

    void SetCubemap(bool isCubemap) { isCubemap_ = isCubemap; }
    bool IsCubemap() const { return isCubemap_; }

private:
    using Layer = Vector<u8>; // TODO: Vector

    bool Init(ktxTexture* texture);
    bool Add(u32 layer, ktxTexture* texture);

    AllocatorRef allocator_;
    u32 width_{};
    u32 height_{};
    u32 pixelSize_{ 4 };
    Vector<Layer> layers_;
    bool isCubemap_{};
};

} // namespace ugine