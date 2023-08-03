#include "VulkanInitializers.h"

#include <ugine/Color.h>

#include <ugine/Ugine.h>

namespace ugine::gfxapi {

vk::DescriptorSetLayoutBinding ShaderLayoutBinding(vk::ShaderStageFlags stage, vk::DescriptorType type, u32 binding, u32 count) {
    vk::DescriptorSetLayoutBinding dsBinding{};
    dsBinding.stageFlags = stage;
    dsBinding.descriptorType = type;
    dsBinding.binding = binding;
    dsBinding.descriptorCount = count;
    return dsBinding;
}

vk::WriteDescriptorSet DescriptorBuffer(
    vk::DescriptorSet descriptorSet, vk::DescriptorType type, const vk::DescriptorBufferInfo* info, u32 binding, u32 count, u32 arrayElement) {
    vk::WriteDescriptorSet descriptorWrite{};

    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = arrayElement;
    descriptorWrite.descriptorType = type;
    descriptorWrite.descriptorCount = count;
    descriptorWrite.pBufferInfo = info;

    return descriptorWrite;
}

vk::WriteDescriptorSet DescriptorBuffer(
    vk::DescriptorSet descriptorSet, vk::DescriptorType type, const std::vector<vk::DescriptorBufferInfo>& info, u32 binding, u32 count, u32 arrayElement) {
    vk::WriteDescriptorSet descriptorWrite{};

    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = arrayElement;
    descriptorWrite.descriptorType = type;
    descriptorWrite.descriptorCount = count;
    descriptorWrite.pBufferInfo = info.data();

    return descriptorWrite;
}

vk::WriteDescriptorSet DescriptorImage(
    vk::DescriptorSet descriptorSet, vk::DescriptorType type, const vk::DescriptorImageInfo* info, u32 binding, u32 count, u32 arrayElement) {
    vk::WriteDescriptorSet descriptorWrite{};

    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = arrayElement;
    descriptorWrite.descriptorType = type;
    descriptorWrite.descriptorCount = count;
    descriptorWrite.pImageInfo = info;

    return descriptorWrite;
}

vk::WriteDescriptorSet DescriptorImage(
    vk::DescriptorSet descriptorSet, vk::DescriptorType type, const std::vector<vk::DescriptorImageInfo>& info, u32 binding, u32 arrayElement, size_t size) {
    vk::WriteDescriptorSet descriptorWrite{};

    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = arrayElement;
    descriptorWrite.descriptorType = type;
    descriptorWrite.descriptorCount = static_cast<u32>(size > 0 ? size : info.size());
    descriptorWrite.pImageInfo = info.data();

    return descriptorWrite;
}

const char* ToString(vk::SampleCountFlagBits samples) {
    switch (samples) {
    case vk::SampleCountFlagBits::e1: return "1x";
    case vk::SampleCountFlagBits::e2: return "2x";
    case vk::SampleCountFlagBits::e4: return "4x";
    case vk::SampleCountFlagBits::e8: return "8x";
    case vk::SampleCountFlagBits::e16: return "16x";
    case vk::SampleCountFlagBits::e32: return "32x";
    case vk::SampleCountFlagBits::e64: return "64x";
    default: UGINE_ASSERT(false); return "???";
    }
}

vk::ClearValue ClearColor(f32 r, f32 g, f32 b, f32 a) {
    return vk::ClearValue{}.setColor(vk::ClearColorValue(std::array<f32, 4>{ r, g, b, a }));
}

vk::ClearValue ClearDepthStencil(f32 depth, u32 stencil) {
    return vk::ClearValue{}.setDepthStencil(vk::ClearDepthStencilValue(depth, stencil));
}

vk::ClearValue ClearColor(const ColorRGBA& color) {
    return vk::ClearValue{}.setColor(vk::ClearColorValue(std::array<f32, 4>{ color.r, color.g, color.b, color.a }));
}

vk::ClearValue ClearColorDepthStencil(const ColorRGBA& color, f32 depth, u32 stencil) {
    return vk::ClearValue{}
        .setColor(vk::ClearColorValue(std::array<f32, 4>{ color.r, color.g, color.b, color.a }))
        .setDepthStencil(vk::ClearDepthStencilValue(depth, stencil));
}

} // namespace ugine::gfxapi
