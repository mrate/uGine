#pragma once

#include <nlohmann/json.hpp>

#include <vulkan/vulkan.hpp>

namespace vk {
NLOHMANN_JSON_SERIALIZE_ENUM(PolygonMode,
    {
        { PolygonMode::eFill, "fill" },
        { PolygonMode::eFillRectangleNV, "fillRectangleNV" },
        { PolygonMode::eLine, "line" },
        { PolygonMode::ePoint, "point" },
    })

NLOHMANN_JSON_SERIALIZE_ENUM(CullModeFlagBits,
    {
        { CullModeFlagBits::eBack, "back" },
        { CullModeFlagBits::eFront, "front" },
        { CullModeFlagBits::eFrontAndBack, "frontAndBack" },
        { CullModeFlagBits::eNone, "none" },
    })

NLOHMANN_JSON_SERIALIZE_ENUM(BlendOp,
    {
        { BlendOp::eAdd, "add" },
        { BlendOp::eSubtract, "subtract" },
        { BlendOp::eReverseSubtract, "reverseSubtract" },
        { BlendOp::eMin, "min" },
        { BlendOp::eMax, "max" },
    })

NLOHMANN_JSON_SERIALIZE_ENUM(BlendFactor,
    {
        { BlendFactor::eZero, "zero" },
        { BlendFactor::eOne, "one" },
        { BlendFactor::eSrcColor, "srcColor" },
        { BlendFactor::eOneMinusSrcColor, "oneMinusSrcColor" },
        { BlendFactor::eDstColor, "dstColor" },
        { BlendFactor::eOneMinusDstColor, "oneMinusDstColor" },
        { BlendFactor::eSrcAlpha, "srcAlpha" },
        { BlendFactor::eOneMinusSrcAlpha, "oneMinusSrcAlpha" },
        { BlendFactor::eDstAlpha, "dstAlpha" },
        { BlendFactor::eOneMinusDstAlpha, "oneMinusDstAlpha" },
        { BlendFactor::eConstantColor, "constantColor" },
        { BlendFactor::eOneMinusConstantColor, "oneMinusConstantColor" },
        { BlendFactor::eConstantAlpha, "constantAlpha" },
        { BlendFactor::eOneMinusConstantAlpha, "oneMinusConstantAlpha" },
        { BlendFactor::eSrcAlphaSaturate, "srcAlphaSaturate" },
        { BlendFactor::eSrc1Color, "src1Color" },
        { BlendFactor::eOneMinusSrc1Color, "oneMinusSrc1Color" },
        { BlendFactor::eSrc1Alpha, "src1Alpha" },
        { BlendFactor::eOneMinusSrc1Alpha, "oneMinusSrc1Alpha" },
    })

NLOHMANN_JSON_SERIALIZE_ENUM(SampleCountFlagBits,
    {
        { SampleCountFlagBits::e1, "1" },
        { SampleCountFlagBits::e2, "2" },
        { SampleCountFlagBits::e4, "4" },
        { SampleCountFlagBits::e8, "8" },
        { SampleCountFlagBits::e8, "8" },
        { SampleCountFlagBits::e16, "16" },
        { SampleCountFlagBits::e32, "32" },
        { SampleCountFlagBits::e64, "64" },
    })

NLOHMANN_JSON_SERIALIZE_ENUM(CompareOp,
    {
        { CompareOp::eNever, "never" },
        { CompareOp::eLess, "less" },
        { CompareOp::eEqual, "equal" },
        { CompareOp::eLessOrEqual, "lessOrEqual" },
        { CompareOp::eGreater, "greater" },
        { CompareOp::eNotEqual, "notEqual" },
        { CompareOp::eGreaterOrEqual, "greaterOrEqual" },
        { CompareOp::eAlways, "always" },
    })

NLOHMANN_JSON_SERIALIZE_ENUM(StencilOp,
    {
        { StencilOp::eKeep, "keep" },
        { StencilOp::eZero, "zero" },
        { StencilOp::eReplace, "replace" },
        { StencilOp::eIncrementAndClamp, "incrementAndClamp" },
        { StencilOp::eDecrementAndClamp, "decrementAndClamp" },
        { StencilOp::eInvert, "invert" },
        { StencilOp::eIncrementAndWrap, "incrementAndWrap" },
        { StencilOp::eDecrementAndWrap, "decrementAndWrap" },
    })

NLOHMANN_JSON_SERIALIZE_ENUM(PrimitiveTopology,
    {
        { PrimitiveTopology::ePointList, "pointList" },
        { PrimitiveTopology::eLineList, "lineList" },
        { PrimitiveTopology::eLineStrip, "lineStrip" },
        { PrimitiveTopology::eTriangleList, "triangleList" },
        { PrimitiveTopology::eTriangleStrip, "triangleStrip" },
        { PrimitiveTopology::eTriangleFan, "triangleFan" },
        { PrimitiveTopology::eLineListWithAdjacency, "lineListWithAdjacency" },
        { PrimitiveTopology::eLineStripWithAdjacency, "lineStripWithAdjacency" },
        { PrimitiveTopology::eTriangleListWithAdjacency, "triangleListWithAdjacency" },
        { PrimitiveTopology::eTriangleStripWithAdjacency, "triangleStripWithAdjacency" },
        { PrimitiveTopology::ePatchList, "patchList" },
    })
} // namespace vk
