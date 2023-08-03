#pragma once

#include <vulkan/vulkan.hpp>

#include <ugine/Ugine.h>

namespace ugine {
struct ColorRGBA;
}

namespace ugine::gfxapi {

vk::DescriptorSetLayoutBinding ShaderLayoutBinding(vk::ShaderStageFlags stage, vk::DescriptorType type, u32 binding, u32 count = 1);

vk::WriteDescriptorSet DescriptorBuffer(
    vk::DescriptorSet descriptorSet, vk::DescriptorType type, const vk::DescriptorBufferInfo* info, u32 binding, u32 count = 1, u32 arrayElement = 0);

vk::WriteDescriptorSet DescriptorImage(
    vk::DescriptorSet descriptorSet, vk::DescriptorType type, const vk::DescriptorImageInfo* info, u32 binding, u32 count = 1, u32 arrayElement = 0);

const char* ToString(vk::SampleCountFlagBits samples);

vk::ClearValue ClearColor(f32 r, f32 g, f32 b, f32 a);
vk::ClearValue ClearColor(const ColorRGBA& color);
vk::ClearValue ClearDepthStencil(f32 depth, u32 stencil);
vk::ClearValue ClearColorDepthStencil(const ColorRGBA& color, f32 depth, u32 stencil);

//void EnsureCommand(GPUCommandList& cmd);

} // namespace ugine::gfxapi
