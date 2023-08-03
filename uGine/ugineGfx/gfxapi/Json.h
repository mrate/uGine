#pragma once

#include <ugine/Color.h>

#include <gfxapi/Types.h>

#include <nlohmann/json.hpp>

namespace ugine::gfxapi {

NLOHMANN_JSON_SERIALIZE_ENUM(TextureLayout,
    {
        { TextureLayout::Undefined, "Undefined" },
        { TextureLayout::General, "General" },
        { TextureLayout::ShaderReadOnly, "ShaderReadOnly" },
        { TextureLayout::ColorAttachment, "ColorAttachment" },
        { TextureLayout::DepthStencilAttachment, "DepthStencilAttachment" },
        { TextureLayout::DepthStencilReadOnly, "DepthStencilReadOnly" },
        { TextureLayout::DepthAttachment, "DepthAttachment" },
        { TextureLayout::DepthReadOnly, "DepthReadOnly" },
        { TextureLayout::StencilAttachment, "StencilAttachment" },
        { TextureLayout::StencilReadOnly, "StencilReadOnly" },
        { TextureLayout::TransferSrc, "TransferSrc" },
        { TextureLayout::TransferDst, "TransferDst" },
        { TextureLayout::Present, "Present" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(TextureFlags,
    {
        { TextureFlags::None, "None" },
        { TextureFlags::CpuRead, "CpuRead" },
        { TextureFlags::CpuWrite, "CpuWrite" },
        { TextureFlags::ShaderResource, "ShaderResource" },
        { TextureFlags::RenderTarget, "RenderTarget" },
        { TextureFlags::UnorderedAccess, "UnorderedAccess" },
        { TextureFlags::Shared, "Shared" },
        { TextureFlags::GdiCompatible, "GdiCompatible" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(TextureDimension,
    {
        { TextureDimension::Texture2D, "Texture2D" },
        { TextureDimension::Texture3D, "Texture3D" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(BufferFlags,
    {
        { BufferFlags::None, "None" },
        { BufferFlags::Uniform, "Uniform" },
        { BufferFlags::Storage, "Storage" },
        { BufferFlags::UnorderedAccess, "UnorderedAccess" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(TextureUsageFlags,
    {
        { TextureUsageFlags::Sampled, "Sampled" },
        { TextureUsageFlags::Storage, "Storage" },
        { TextureUsageFlags::RenderTarget, "RenderTarget" },
        { TextureUsageFlags::DepthStencil, "DepthStencil" },
        { TextureUsageFlags::TransferSrc, "TransferSrc" },
        { TextureUsageFlags::TransferDst, "TransferDst" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(TextureMiscFlags,
    {
        { TextureMiscFlags::Share, "Share" },
        { TextureMiscFlags::ShareLock, "ShareLock" },
        { TextureMiscFlags::Cube, "Cube" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(StencilFaceFlags,
    {
        { StencilFaceFlags::Front, "Front" },
        { StencilFaceFlags::Back, "Back" },
        { StencilFaceFlags::FrontAndBack, "FrontAndBack" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(IndexType,
    {
        { IndexType::Uint16, "Uint16" },
        { IndexType::Uint32, "Uint32" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(Format,
    {
        { Format::Unknown, "Unknown" },
        { Format::R8_Uint, "R8_Uint" },
        { Format::R8G8_Uint, "R8G8_Uint" },
        { Format::R8G8B8A8_Uint, "R8G8B8A8_Uint" },
        { Format::R8_Unorm, "R8_Unorm" },
        { Format::R8G8_Unorm, "R8G8_Unorm" },
        { Format::R8G8B8A8_Unorm, "R8G8B8A8_Unorm" },
        { Format::R8G8B8A8_Unorm_Srgb, "R8G8B8A8_Unorm_Srgb" },
        { Format::R16_Uint, "R16_Uint" },
        { Format::R16G16_Uint, "R16G16_Uint" },
        { Format::R16G16B16A16_Uint, "R16G16B16A16_Uint" },
        { Format::R32_Uint, "R32_Uint" },
        { Format::R32G32_Uint, "R32G32_Uint" },
        { Format::R32G32B32_Uint, "R32G32B32_Uint" },
        { Format::R32G32B32A32_Uint, "R32G32B32A32_Uint" },
        { Format::R32_Float, "R32_Float" },
        { Format::R32G32_Float, "R32G32_Float" },
        { Format::R32G32B32_Float, "R32G32B32_Float" },
        { Format::R32G32B32A32_Float, "R32G32B32A32_Float" },
        { Format::B8G8R8A8_Unorm, "B8G8R8A8_Unorm" },
        { Format::B8G8R8A8_Unorm_Srgb, "B8G8R8A8_Unorm_Srgb" },
        { Format::R16G16B16A16_Float, "R16G16B16A16_Float" },
        { Format::D24_Unorm_S8_Uint, "D24_Unorm_S8_Uint" },
        { Format::D32_Float, "D32_Float" },
        { Format::COUNT, "COUNT" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(ComparisonFunc,
    {
        { ComparisonFunc::Never, "Never" },
        { ComparisonFunc::Less, "Less" },
        { ComparisonFunc::Equal, "Equal" },
        { ComparisonFunc::LessEqual, "LessEqual" },
        { ComparisonFunc::Greater, "Greater" },
        { ComparisonFunc::NotEqual, "NotEqual" },
        { ComparisonFunc::GreaterEqual, "GreaterEqual" },
        { ComparisonFunc::Always, "Always" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(StencilOp,
    {
        { StencilOp::Keep, "Keep" },
        { StencilOp::Zero, "Zero" },
        { StencilOp::Replace, "Replace" },
        { StencilOp::IncrementAndClamp, "IncrementAndClamp" },
        { StencilOp::DecrementAndClamp, "DecrementAndClamp" },
        { StencilOp::Invert, "Invert" },
        { StencilOp::IncrementAndWrap, "IncrementAndWrap" },
        { StencilOp::DecrementAndWrap, "DecrementAndWrap" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(DepthWriteMask,
    {
        { DepthWriteMask::Zero, "Zero" },
        { DepthWriteMask::All, "All" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(CullMode,
    {
        { CullMode::None, "None" },
        { CullMode::Front, "Front" },
        { CullMode::Back, "Back" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(FillMode,
    {
        { FillMode::Fill, "Fill" },
        { FillMode::Line, "Line" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(Filter,
    {
        { Filter::Nearest, "Nearest" },
        { Filter::Linear, "Linear" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(TextureAddressMode,
    {
        { TextureAddressMode::Wrap, "Wrap" },
        { TextureAddressMode::Mirror, "Mirror" },
        { TextureAddressMode::Clamp, "Clamp" },
        { TextureAddressMode::Border, "Border" },
        { TextureAddressMode::MirrorOnce, "MirrorOnce" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(Blend,
    {
        { Blend::Zero, "Zero" },
        { Blend::One, "One" },
        { Blend::SrcColor, "SrcColor" },
        { Blend::OneMinusSrcColor, "OneMinusSrcColor" },
        { Blend::SrcAlpha, "SrcAlpha" },
        { Blend::OneMinusSrcAlpha, "OneMinusSrcAlpha" },
        { Blend::DstAlpha, "DstAlpha" },
        { Blend::OneMinusDstAlpha, "OneMinusDstAlpha" },
        { Blend::DstColor, "DstColor" },
        { Blend::OneMinusDstColor, "OneMinusDstColor" },
        { Blend::SrcAlphaSat, "SrcAlphaSat" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(BlendOp,
    {
        { BlendOp::Add, "Add" },
        { BlendOp::Subtract, "Subtract" },
        { BlendOp::RevSubtract, "RevSubtract" },
        { BlendOp::Min, "Min" },
        { BlendOp::Max, "Max" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(BindFlags,
    {
        { BindFlags::None, "None" },
        { BindFlags::VertexBuffer, "VertexBuffer" },
        { BindFlags::IndexBuffer, "IndexBuffer" },
        { BindFlags::ConstantBuffer, "ConstantBuffer" },
        { BindFlags::ShaderResource, "ShaderResource" },
        { BindFlags::RenderTarget, "RenderTarget" },
        { BindFlags::UnorderedAccess, "UnorderedAccess" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(Usage,
    {
        { Usage::Default, "Default" },
        { Usage::Immutable, "Immutable" },
        { Usage::Dynamic, "Dynamic" },
        { Usage::Staging, "Staging" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(CpuAccessFlags,
    {
        { CpuAccessFlags::None, "None" },
        { CpuAccessFlags::Write, "Write" },
        { CpuAccessFlags::Read, "Read" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(AttachmentLoadOp,
    {
        { AttachmentLoadOp::Load, "Load" },
        { AttachmentLoadOp::Clear, "Clear" },
        { AttachmentLoadOp::DontCare, "DontCare" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(AttachmentStoreOp,
    {
        { AttachmentStoreOp::Store, "Store" },
        { AttachmentStoreOp::DontCare, "DontCare" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(PrimitiveTopology,
    {
        { PrimitiveTopology::TriangleList, "TriangleList" },
        { PrimitiveTopology::LineList, "LineList" },
        { PrimitiveTopology::PointList, "PointList" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(InputSlotClass,
    {
        { InputSlotClass::PerVertex, "PerVertex" },
        { InputSlotClass::PerInstance, "PerInstance" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(AccessFlags,
    {
        { AccessFlags::ShaderRead, "ShaderRead" },
        { AccessFlags::ShaderWrite, "ShaderWrite" },
    });
NLOHMANN_JSON_SERIALIZE_ENUM(ShaderStage,
    {
        { ShaderStage::VertexShader, "VertexShader" },
        { ShaderStage::HullShader, "HullShader" },
        { ShaderStage::DomainShader, "DomainShader" },
        { ShaderStage::GeometryShader, "GeometryShader" },
        { ShaderStage::FragmentShader, "FragmentShader" },
        { ShaderStage::ComputeShader, "ComputeShader" },
    });
} // namespace ugine::gfxapi