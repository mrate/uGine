#pragma once

#include "Types.h"

#include <string_view>

namespace ugine::gfxapi {

inline BufferDesc UniformBuffer(std::string_view name, size_t size) {
    return BufferDesc{
        .name = name.data(),
        .flags = BufferFlags::Uniform,
        .size = static_cast<u32>(size),
        .cpuAccess = CpuAccessFlags::Write,
    };
}

template <typename T> inline BufferDesc UniformBuffer(std::string_view name) {
    return UniformBuffer(name, sizeof(T));
}

inline BufferDesc StorageBuffer(std::string_view name, size_t elementSize, u32 elementCount, CpuAccessFlags cpuAccess = CpuAccessFlags::None) {
    return BufferDesc{
        .name = name.data(),
        .flags = BufferFlags::Storage,
        .size = static_cast<u32>(elementSize * elementCount),
        .cpuAccess = CpuAccessFlags::Write,
    };
}

template <typename T> inline BufferDesc StorageBuffer(std::string_view name, u32 elementCount, CpuAccessFlags cpuAccess = CpuAccessFlags::None) {
    return StorageBuffer(name, sizeof(T), elementCount, cpuAccess);
}

inline SamplerDesc Sampler(std::string_view name, TextureAddressMode addressModeUVW, Filter minMagFilter, Filter mipmapFilter, f32 anisotropy = 16.0f) {
    return SamplerDesc{
        .name = name.data(),
        .mipmapFilter = mipmapFilter,
        .minFilter = minMagFilter,
        .magFilter = minMagFilter,
        .addressU = addressModeUVW,
        .addressV = addressModeUVW,
        .addressW = addressModeUVW,
        .maxAnisotropy = anisotropy,
    };
}

} // namespace ugine::gfxapi