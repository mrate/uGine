#pragma once

#include <ugineGfxConfig.h>

#include "Handle.h"

#include <ugine/ArrayProxy.h>
#include <ugine/Color.h>
#include <ugine/Utils.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ugine::gfxapi {

constexpr u32 MaxColorAttachments{ 8 };
constexpr u32 MaxViewports{ 8 };
constexpr u32 MaxDatasets{ 16 };
constexpr u32 MaxBindings{ 16 };
constexpr u32 MaxVertexBindings{ 2 };

constexpr i32 BindlessInvalid{ -1 };

enum class TextureLayout : u8 {
    Undefined = 0,
    General,
    TransferSrc,
    TransferDst,
    Present,
    Attachment,
    ReadOnly,
};

enum class TextureFlags : u32 {
    None = 0,
    CpuRead = UGINE_BIT(0),
    CpuWrite = UGINE_BIT(1),
    ShaderResource = UGINE_BIT(2),
    RenderTarget = UGINE_BIT(3),
    UnorderedAccess = UGINE_BIT(4),
    Shared = UGINE_BIT(5),
    GdiCompatible = UGINE_BIT(6),
};

UGINE_FLAGS(TextureFlags, u32);

enum class TextureDimension : u8 {
    Texture2D = 0,
    Texture3D,
};

enum class BufferFlags : u32 {
    None = 0,
    Uniform = UGINE_BIT(0),
    Storage = UGINE_BIT(1),
    Vertex = UGINE_BIT(2),
    Index = UGINE_BIT(3),
    Indirect = UGINE_BIT(4),
};

UGINE_FLAGS(BufferFlags, u32);

enum class TextureUsageFlags : u32 {
    Sampled = UGINE_BIT(0),
    Storage = UGINE_BIT(1),
    RenderTarget = UGINE_BIT(2),
    DepthStencil = UGINE_BIT(3),
    TransferSrc = UGINE_BIT(4),
    TransferDst = UGINE_BIT(5),
};

UGINE_FLAGS(TextureUsageFlags, u32);

enum class TextureMiscFlags : u32 {
    Cube = UGINE_BIT(1),
};

UGINE_FLAGS(TextureMiscFlags, u32);

enum class StencilFaceFlags : u32 {
    Front = UGINE_BIT(0),
    Back = UGINE_BIT(1),
    FrontAndBack = UGINE_BIT(2),
};

UGINE_FLAGS(StencilFaceFlags, u32);

enum class AccessFlags : u32 {
    IndirectCommandRead = UGINE_BIT(0),
    IndexRead = UGINE_BIT(1),
    VertexAttributeRead = UGINE_BIT(2),
    UniformRead = UGINE_BIT(3),
    InputAttachmentRead = UGINE_BIT(4),
    ShaderRead = UGINE_BIT(5),
    ShaderWrite = UGINE_BIT(6),
    ColorAttachmentRead = UGINE_BIT(7),
    ColorAttachmentWrite = UGINE_BIT(8),
    DepthStencilAttachmentRead = UGINE_BIT(9),
    DepthStencilAttachmentWrite = UGINE_BIT(10),
    TransferRead = UGINE_BIT(11),
    TransferWrite = UGINE_BIT(12),
    HostRead = UGINE_BIT(13),
    HostWrite = UGINE_BIT(14),
    MemoryRead = UGINE_BIT(15),
    MemoryWrite = UGINE_BIT(16),
    None = UGINE_BIT(17),
};

UGINE_FLAGS(AccessFlags, u32);

enum class IndexType : u8 {
    Uint16 = 0,
    Uint32,
};

enum class Format : u8 {
    Unknown = 0,
    R8_Uint,
    R8G8_Uint,
    R8G8B8A8_Uint,
    R8_Unorm,
    R8G8_Unorm,
    R8G8B8A8_Unorm,
    R8G8B8A8_Unorm_Srgb,
    R16_Uint,
    R16G16_Uint,
    R16G16B16A16_Uint,
    R32_Uint,
    R32G32_Uint,
    R32G32B32_Uint,
    R32G32B32A32_Uint,
    R32_Float,
    R32G32_Float,
    R32G32B32_Float,
    R32G32B32A32_Float,
    B8G8R8A8_Unorm,
    B8G8R8A8_Unorm_Srgb,
    R16G16B16A16_Float,
    D24_Unorm_S8_Uint,
    D32_Float,
    D16_Unorm_S8_Uint,
    D16_Unorm,
    D32_Float_S8_Uint,

    COUNT,
};

enum class ComparisonFunc : u8 {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

enum class StencilOp : u8 {
    Keep = 0,
    Zero,
    Replace,
    IncrementAndClamp,
    DecrementAndClamp,
    Invert,
    IncrementAndWrap,
    DecrementAndWrap,
};

enum class DepthWriteMask : u8 {
    Zero = 0,
    All,
};

enum class CullMode : u8 {
    None = 0,
    Front,
    Back,
};

enum class FillMode : u8 {
    Fill = 0,
    Line,
};

enum class Filter : u8 {
    Nearest = 0,
    Linear,
};

enum class TextureAddressMode : u8 {
    Wrap = 0,
    Mirror,
    Clamp,
    Border,
    MirrorOnce,
};

struct StencilOpDesc {
    StencilOp failOp{ StencilOp::Zero };
    StencilOp passOp{ StencilOp::Zero };
    ComparisonFunc compareOp{ ComparisonFunc::Always };
    StencilOp depthFailOp{ StencilOp::Zero };
    u32 reference{};
    u8 writeMask{ 0xff };
    u8 compareMask{ 0xff };
};

struct DepthStencilDesc {
    bool depthTestEnable{};
    bool depthWriteEnable{ true };
    DepthWriteMask depthWriteMask{ DepthWriteMask::All };
    ComparisonFunc depthFunc{ ComparisonFunc::LessEqual };
    bool stencilEnable{};
    StencilOpDesc frontFace{};
    StencilOpDesc backFace{};
};

struct RasterizerDesc {
    bool frontCCW{};
    CullMode cullMode{ CullMode::None };
    FillMode fillMode{ FillMode::Fill };
    bool depthBiasEnable{};
    float depthBiasConstantFactor{};
    float depthBiasSlopeFactor{};
};

enum class Blend : u8 {
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    DstColor,
    OneMinusDstColor,
    SrcAlphaSat,
};

enum class BlendOp : u8 {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,
};

struct BlendRenderTarget {
    bool enable{};

    Blend srcBlend{ Blend::One };
    Blend dstBlend{ Blend::Zero };
    BlendOp blendOp{ BlendOp::Add };
    Blend srcAlphaBlend{ Blend::SrcAlpha };
    Blend dstAlphaBlend{ Blend::OneMinusSrcAlpha };
    BlendOp alphaBlendOp{ BlendOp::Add };
};

struct BlendDesc {
    std::array<BlendRenderTarget, MaxColorAttachments> rtv;
};

struct SamplerDesc {
    std::string name;
    Filter mipmapFilter{ Filter::Linear };
    Filter minFilter{ Filter::Linear };
    Filter magFilter{ Filter::Linear };
    TextureAddressMode addressU{ TextureAddressMode::Clamp };
    TextureAddressMode addressV{ TextureAddressMode::Clamp };
    TextureAddressMode addressW{ TextureAddressMode::Clamp };
    f32 mipLODBias{};
    f32 maxAnisotropy{ 1.0f };
    ComparisonFunc comparisonFunc{ ComparisonFunc::Always };
    std::array<f32, 4> borderColor;
    f32 minLOD{};
    f32 maxLOD{};
};

enum class BindFlags : u32 {
    None = 0,
    VertexBuffer = UGINE_BIT(0),
    IndexBuffer = UGINE_BIT(1),
    ConstantBuffer = UGINE_BIT(2),
    ShaderResource = UGINE_BIT(3),
    RenderTarget = UGINE_BIT(4),
    UnorderedAccess = UGINE_BIT(5),
};

UGINE_FLAGS(BindFlags, u32);

enum class Usage : u8 {
    Default = 0,
    Immutable,
    Dynamic,
    Staging,
};

enum class CpuAccessFlags : u32 {
    None = 0,
    Write = UGINE_BIT(0),
    Read = UGINE_BIT(1),
};

UGINE_FLAGS(CpuAccessFlags, u32);

struct TextureCoords {
    f32 u{ 0.0f };
    f32 v{ 0.0f };
    f32 um{ 1.0f };
    f32 vm{ 1.0f };
};

inline bool operator==(const TextureCoords& lhs, const TextureCoords& rhs) {
    return (lhs.u == rhs.u && lhs.v == rhs.v && lhs.um == rhs.um && lhs.vm == rhs.vm);
}

inline bool operator!=(const TextureCoords& lhs, const TextureCoords& rhs) {
    return !(lhs == rhs);
}

struct SwapchainDesc {
    u32 minBufferCount{};
    u32 width{};
    u32 height{};
    Format format{ Format::Unknown };
    struct {
        bool enabled{ true };
        int numerator{};
        int denominator{ 1 };
    } vsync;

    struct {
        void* hInstance{};
        void* hWnd{};
    } win32;

    bool windowed{};
    bool discard{};
};

enum class AttachmentLoadOp : u8 {
    Load = 0,
    Clear,
    DontCare,
};

enum class AttachmentStoreOp : u8 {
    Store = 0,
    DontCare,
};

enum class PipelineStageFlags : u32 {
    TopOfPipe = UGINE_BIT(0),
    DrawIndirect = UGINE_BIT(1),
    VertexInput = UGINE_BIT(2),
    VertexShader = UGINE_BIT(3),
    TessellationControlShader = UGINE_BIT(4),
    TessellationEvaluationShader = UGINE_BIT(5),
    GeometryShader = UGINE_BIT(6),
    FragmentShader = UGINE_BIT(7),
    EarlyFragmentTests = UGINE_BIT(8),
    LateFragmentTests = UGINE_BIT(9),
    ColorAttachmentOutput = UGINE_BIT(10),
    ComputeShader = UGINE_BIT(11),
    Transfer = UGINE_BIT(12),
    BottomOfPipe = UGINE_BIT(13),
    Host = UGINE_BIT(14),
    AllGraphics = UGINE_BIT(15),
    AllCommands = UGINE_BIT(16),
    None = UGINE_BIT(17),
    TransformFeedback = UGINE_BIT(18),
    ConditionalRendering = UGINE_BIT(19),
    AccelerationStructureBuild = UGINE_BIT(20),
    RayTracingShader = UGINE_BIT(21),
    FragmentDensityProcess = UGINE_BIT(22),
    FragmentShadingRateAttachment = UGINE_BIT(23),
    CommandPreprocess = UGINE_BIT(24),
    TaskShader = UGINE_BIT(25),
    MeshShader = UGINE_BIT(26),
};

UGINE_FLAGS(PipelineStageFlags, u32);

struct RenderPassDependency {
    AccessFlags srcAccessFlags{};
    PipelineStageFlags srcStageMask{};
    AccessFlags dstAccessFlags{};
    PipelineStageFlags dstStageMask{};
};

struct RenderPassAttachmentDesc {
    Format format{};
    AttachmentLoadOp loadOp{ AttachmentLoadOp::Clear };
    AttachmentStoreOp storeOp{ AttachmentStoreOp::Store };
    AttachmentLoadOp stencilLoadOp{ AttachmentLoadOp::DontCare };
    AttachmentStoreOp stencilStoreOp{ AttachmentStoreOp::DontCare };
    TextureLayout initialLayout{ TextureLayout::Undefined };
    TextureLayout finalLayout{ TextureLayout::Attachment };
};

struct RenderPassDesc {
    const char* name{};
    u32 colorAttachmentCount{};
    std::array<RenderPassAttachmentDesc, MaxColorAttachments> colorAttachments;
    std::optional<RenderPassAttachmentDesc> depthAttachment{};
    // TODO: Experimental:
    std::optional<RenderPassDependency> inputDependency;
    std::optional<RenderPassDependency> outputDependency;
};

struct CompiledShader {
    const char* name{};
    const char* entryPoint{};
    const void* data{};
    size_t size{};
};

enum class PrimitiveTopology : u8 {
    TriangleList = 0,
    LineList,
    PointList,
    LineStrip,
};

enum class InputSlotClass : u8 {
    PerVertex = 0,
    PerInstance,
};

struct VertexAttribute {
    u32 group{};
    u32 location{};
    u32 offset{};
    Format format{ Format::R32_Float };
    InputSlotClass inputSlotClass{ InputSlotClass::PerVertex };
};

struct VertexBindings {
    u32 dataStride{};
    InputSlotClass slot{ InputSlotClass::PerVertex };
};

struct InputAssemblyDesc {
    PrimitiveTopology primitiveTopology{};
    const VertexAttribute* vertexAttributes{};
    u32 vertexAttributesCount{};
    u32 vertexBindingsCount{};
    std::array<VertexBindings, MaxVertexBindings> vertexBindings;
};

struct Extent2D {
    u32 width{};
    u32 height{};
};

inline bool operator==(const Extent2D& a, const Extent2D& b) {
    return a.width == b.width && a.height == b.height;
}

inline bool operator!=(const Extent2D& a, const Extent2D& b) {
    return !(a == b);
}

struct Extent3D {
    u32 width{};
    u32 height{};
    u32 depth{};

    Extent3D() = default;
    Extent3D(u32 width, u32 height, u32 depth)
        : width{ width }
        , height{ height }
        , depth{ depth } {}
    Extent3D(const Extent2D& extent2d, u32 depth = 1)
        : width{ extent2d.width }
        , height{ extent2d.height }
        , depth{ depth } {}
};

inline bool operator==(const Extent3D& a, const Extent3D& b) {
    return a.width == b.width && a.height == b.height && a.depth == b.depth;
}

inline bool operator!=(const Extent3D& a, const Extent3D& b) {
    return !(a == b);
}

struct Point2D {
    i32 x{};
    i32 y{};
};

struct Rect2D {
    Point2D offset{};
    Extent2D extent{};

    static Rect2D FromValues(i32 x, i32 y, u32 width, u32 height) { return Rect2D{ { x, y }, { width, height } }; }

    static Rect2D FromExtent(const Extent2D& extent) { return Rect2D{ Point2D{}, extent }; }
};

struct Viewport {
    f32 x{};
    f32 y{};
    f32 width{};
    f32 height{};
    f32 minDepth{};
    f32 maxDepth{ 1.0f };

    static Viewport FromSize(f32 width, f32 height) { return Viewport{ 0, 0, width, height }; }
    static Viewport FromExtent(const Extent2D& extent) { return Viewport{ 0, 0, static_cast<f32>(extent.width), static_cast<f32>(extent.height) }; }
};

struct TextureDesc {
    std::string name;
    Extent3D extent{};
    u32 arrayLayers{ 1 };
    Format format{};
    TextureUsageFlags usage{};
    TextureMiscFlags misc{};
    u32 mipLevels{ 1 };
    bool generateMips{};
};

struct SubresourceData {
    const void* data{};
    u64 size{};
    u32 pitch{};
    u32 slicePitch{};
};

//enum class TextureViewType {
//    Texture2D,
//    Texture2DArray,
//    Texture3D,
//    TextureCube,
//};
//

enum class TextureAspectFlags : u8 {
    Color = UGINE_BIT(0),
    Depth = UGINE_BIT(1),
    Stencil = UGINE_BIT(2),
};

UGINE_FLAGS(TextureAspectFlags, u8);

//struct TextureViewDesc {
//    TextureHandle texture{};
//    Format format{};
//    TextureViewType type{ TextureViewType::Texture2D };
//    TextureAspectFlags aspect{ TextureAspectFlags::Color };
//    u32 baseMipLevel{};
//    u32 mipLevelCount{ 1 };
//    u32 baseArrayLayer{};
//    u32 layerCount{ 1 };
//};

struct BufferDesc {
    std::string name;
    BufferFlags flags{};
    u64 size{};
    CpuAccessFlags cpuAccess{ CpuAccessFlags::None };
};

struct BufferCopy {
    u64 srcOffset{};
    u64 dstOffset{};
    u64 size{};
};

//struct BufferViewDesc {
//    BufferHandle buffer{};
//    Format format{};
//    u32 firstElement{ 0 };
//    u32 numElements{ 1 };
//    u32 elementSize{};
//};

struct FramebufferDesc {
    std::string name;
    u32 width{};
    u32 height{};
    u32 layers{ 1 };
    RenderPassHandle renderPass{};
    u32 colorAttachmentCount{};
    std::array<TextureHandle, MaxColorAttachments> colorAttachments;
    TextureHandle depth;
};

enum class ShaderStage : u32 {
    VertexShader = UGINE_BIT(0),
    HullShader = UGINE_BIT(1),
    DomainShader = UGINE_BIT(2),
    GeometryShader = UGINE_BIT(3),
    FragmentShader = UGINE_BIT(4),
    ComputeShader = UGINE_BIT(5),

    All = (VertexShader | HullShader | DomainShader | GeometryShader | FragmentShader | ComputeShader),
};

UGINE_FLAGS(ShaderStage, u32);

// Pipeline.
enum class DynamicPipelineStates : u32 {
    StencilReference = UGINE_BIT(0),
    StencilWriteMask = UGINE_BIT(1),
    StencilCompareMask = UGINE_BIT(2),
};

UGINE_FLAGS(DynamicPipelineStates, u32);

struct GraphicsPipelineDesc {
    std::string name;
    std::optional<CompiledShader> vertexShader{};
    std::optional<CompiledShader> fragmentShader{};
    std::optional<CompiledShader> domainShader{};
    std::optional<CompiledShader> hullShader{};
    std::optional<CompiledShader> geometryShader{};
    RasterizerDesc rasterizerState{};
    BlendDesc blendState{};
    DepthStencilDesc depthStencilState{};
    InputAssemblyDesc inputAssembly{};
    RenderPassHandle renderPass{};
    u32 pushDescriptorDataset{ MaxDatasets };
    DynamicPipelineStates dynamicStates{};
};

struct ComputePipelineDesc {
    std::string name;
    CompiledShader computeShader{};
    u32 pushDescriptorDataset{ MaxDatasets };
};

struct DynamicFramebuffer {
    RenderPassHandle renderPass{};
    Extent2D extent{};
    u32 colorAttachmentCount{};
    std::array<TextureHandle, MaxColorAttachments> colorAttachments;
    TextureHandle depth{};
};

struct BindingSlot {
    enum class Type {
        Uniform,
        ImageSampler,
        Storage,
    };

    Type type{};
    u32 binding{};

    union {
        struct {
            BufferHandle buffer;
            u64 offset;
            u64 size;
        } uniform;

        struct {
            TextureHandle texture;
            SamplerHandle sampler;
        } imageSampler;

        struct {
            BufferHandle buffer;
        } storageBuffer;
    };
};

struct BindingDesc {
    enum class BindingPoint {
        Graphics,
        Compute,
    };

    BindingPoint bindingPoint{ BindingPoint::Graphics };
    // TODO: Pipeline only for dataset layout.
    GraphicsPipelineHandle graphicsPipeline{};
    ComputePipelineHandle computePipeline{};
    u32 set{};
    u32 bindingsCount{};
    std::array<BindingSlot, MaxBindings> bindings;
};

enum class QueryType {
    Timestamp,
};

struct QueryPoolDesc {
    QueryType type{ QueryType::Timestamp };
    u32 count{ 128 };
};

enum class PipelineStage : u32 {
    TopOfPipeline,
    BottomOfPipeline,
};

UGINE_FLAGS(PipelineStage, u32);

} // namespace ugine::gfxapi