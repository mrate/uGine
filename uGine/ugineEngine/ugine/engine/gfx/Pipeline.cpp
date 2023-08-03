#include "Pipeline.h"

#include <ugine/Log.h>

#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/gfx/asset/SerializedMaterial.h>
#include <ugine/engine/shaders/Shader_Material.h>

using namespace ugine::gfxapi;

namespace ugine {

BlendDesc GetBlendDesc(MaterialBlending blending) {
    switch (blending) {
    case MaterialBlending::Opaque:
    case MaterialBlending::Masked:
        return BlendDesc { 
                .rtv = { 
                    BlendRenderTarget{
                        .enable = false,
                        .srcBlend =  Blend::One,
                        .dstBlend =  Blend::Zero,
                        .blendOp =  BlendOp::Add,
                        .srcAlphaBlend =  Blend::One,
                        .dstAlphaBlend =  Blend::One,
                        .alphaBlendOp =  BlendOp::Add,
                    },
                },
            };
    // TODO: Blendings.
    case MaterialBlending::Additive:
        return BlendDesc { 
                .rtv = { 
                    BlendRenderTarget{
                        .enable = true,
                        .srcBlend =  Blend::SrcAlpha,
                        .dstBlend =  Blend::One,
                        .blendOp =  BlendOp::Add,
                        .srcAlphaBlend =  Blend::One,
                        .dstAlphaBlend =  Blend::One,
                        .alphaBlendOp =  BlendOp::Add,
                    },
                },
            };
    // TODO: Blendings.
    case MaterialBlending::Translucent:
        return BlendDesc { 
                .rtv = { 
                    BlendRenderTarget{
                        .enable = true,
                        .srcBlend =  Blend::SrcAlpha,
                        .dstBlend =  Blend::OneMinusSrcAlpha,
                        .blendOp =  BlendOp::Add,
                        .srcAlphaBlend =  Blend::One,
                        .dstAlphaBlend =  Blend::One,
                        .alphaBlendOp =  BlendOp::Add,
                    },
                },
            };
    default: return BlendDesc{};
    }
}

std::vector<VertexAttribute> ParseVertexAttributes(const Vector<Shader::VertexAttribute>& attribs) {
    std::vector<VertexAttribute> attributes{};

    for (const auto& attrib : attribs) {
        VertexAttribute attribute{
            .location = attrib.location,
        };

        if (attrib.name == "in.var.POSITION0" || attrib.name == "in.var.POSITION") {
            attribute.group = 0;
            attribute.offset = offsetof(MaterialVertex, position);
            attribute.format = gfxapi::Format::R32G32B32_Float;
            attribute.inputSlotClass = gfxapi::InputSlotClass::PerVertex;
        } else if (attrib.name == "in.var.NORMAL0" || attrib.name == "in.var.NORMAL") {
            attribute.group = 0;
            attribute.offset = offsetof(MaterialVertex, normal);
            attribute.format = gfxapi::Format::R32G32B32_Float;
            attribute.inputSlotClass = gfxapi::InputSlotClass::PerVertex;
        } else if (attrib.name == "in.var.TANGENT0" || attrib.name == "in.var.TANGENT") {
            attribute.group = 0;
            attribute.offset = offsetof(MaterialVertex, tangent);
            attribute.format = gfxapi::Format::R32G32B32_Float;
            attribute.inputSlotClass = gfxapi::InputSlotClass::PerVertex;
        } else if (attrib.name == "in.var.TEXCOORD0" || attrib.name == "in.var.TEXCOORD") {
            attribute.group = 0;
            attribute.offset = offsetof(MaterialVertex, uv);
            attribute.format = gfxapi::Format::R32G32_Float;
            attribute.inputSlotClass = gfxapi::InputSlotClass::PerVertex;
        } else if (attrib.name == "in.var.POSITION1") {
            attribute.group = 1;
            attribute.offset = offsetof(MaterialVertexInstance, instance0);
            attribute.format = gfxapi::Format::R32G32B32A32_Float;
            attribute.inputSlotClass = gfxapi::InputSlotClass::PerInstance;
        } else if (attrib.name == "in.var.POSITION2") {
            attribute.group = 1;
            attribute.offset = offsetof(MaterialVertexInstance, instance1);
            attribute.format = gfxapi::Format::R32G32B32A32_Float;
            attribute.inputSlotClass = gfxapi::InputSlotClass::PerInstance;
        } else if (attrib.name == "in.var.POSITION3") {
            attribute.group = 1;
            attribute.offset = offsetof(MaterialVertexInstance, instance2);
            attribute.format = gfxapi::Format::R32G32B32A32_Float;
            attribute.inputSlotClass = gfxapi::InputSlotClass::PerInstance;
        } else {
            UGINE_ERROR("Unknown vertex attribute: {}", attrib.name.Data());
            continue;
        }

        attributes.push_back(attribute);
    }

    return attributes;
}

Pipeline::~Pipeline() {
    UGINE_ASSERT(pipelines_.empty());
}

Pipeline::Pipeline(GraphicsState& state, const MaterialPipeline& pipeline, ResourceHandle<Shader> shader) {
    UGINE_ASSERT(shader);

    shader_ = shader;

    switch (pipeline.blending) {
    case MaterialBlending::Masked: materialMask_ = state.SHADER_MASKED_MASK; break;
    case MaterialBlending::Additive:
    case MaterialBlending::Translucent: materialMask_ = state.SHADER_OPACITY_MASK; break;
    }

    for (const auto& [mask, variant] : shader->Variants()) {
        UGINE_DEBUG("-- Creating pipeline for shader {} variant {}", shader->Id().ToString().Data(), mask);

        const auto isDepthMask{ (mask & state.SHADER_DEPTH_PASS_MASK) != 0 };

        // TODO: Stencil.
        DepthStencilDesc depthStencil {
            .depthTestEnable = pipeline.depthTest,
            .depthWriteEnable = pipeline.depthWrite,
            .stencilEnable = true,
            .frontFace = { 
                .failOp = StencilOp::Replace,
                .passOp = StencilOp::Replace,
                .compareOp = ComparisonFunc::Always,
                .depthFailOp = StencilOp::Replace,
                .reference = 1,
                .writeMask = 0xff,
                .compareMask = 0xff,
            },
        };
        depthStencil.backFace = depthStencil.frontFace;

        RasterizerDesc rasterizer{
            .frontCCW = true,
            .cullMode = pipeline.doubleSided ? CullMode::None : CullMode::Back,
            .fillMode = pipeline.wireframe ? FillMode::Line : FillMode::Fill,
        };

        // TODO: Depth bias.
        if (isDepthMask) {
            rasterizer.depthBiasEnable = true;
            rasterizer.depthBiasConstantFactor = 4;
            rasterizer.depthBiasSlopeFactor = 1.5f;
        }

        GraphicsPipelineDesc desc {
            .name = shader->Name().Data(),
            .rasterizerState = rasterizer,
            .blendState = GetBlendDesc(pipeline.blending),
            .depthStencilState = depthStencil,
            .inputAssembly = {
                .primitiveTopology = PrimitiveTopology::TriangleList,
            },
            .renderPass = state.GetRenderPass(isDepthMask ? GraphicsState::RenderPass::Depth : GraphicsState::RenderPass::ForwardHDR), // TODO: Renderpass...
            .pushDescriptorDataset = DATASET_DRAW, // TODO: Last dataset.
            .dynamicStates = DynamicPipelineStates::StencilWriteMask,
        };

        UGINE_ASSERT(desc.renderPass);

        const auto vertexAttributes{ ParseVertexAttributes(variant.vertexAttributes) };

        auto fill = [&](auto& compiled, const auto& binary) {
            compiled.name = shader->Name().Data();
            compiled.entryPoint = binary.entry.Data();
            compiled.data = binary.binary.Data();
            compiled.size = binary.binary.Size();
        };

        CompiledShader vs{};
        CompiledShader hs{};
        CompiledShader ds{};
        CompiledShader gs{};
        CompiledShader fs{};

        for (const auto& [stage, shader] : variant.shaders) {
            switch (stage) {
            case ShaderStage::VertexShader:
                fill(vs, shader);
                desc.vertexShader = vs;
                break;
            case ShaderStage::HullShader:
                fill(hs, shader);
                desc.hullShader = hs;
                break;
            case ShaderStage::DomainShader:
                fill(ds, shader);
                desc.domainShader = ds;
                break;
            case ShaderStage::GeometryShader:
                fill(gs, shader);
                desc.geometryShader = gs;
                break;
            case ShaderStage::FragmentShader:
                fill(fs, shader);
                desc.fragmentShader = fs;
                break;
            }
        }

        desc.inputAssembly.vertexAttributesCount = u32(vertexAttributes.size());
        desc.inputAssembly.vertexAttributes = vertexAttributes.data();

        desc.inputAssembly.vertexBindingsCount = 1;
        desc.inputAssembly.vertexBindings[0].slot = InputSlotClass::PerVertex;
        desc.inputAssembly.vertexBindings[0].dataStride = sizeof(MaterialVertex);

        const auto it{ std::find_if(vertexAttributes.begin(), vertexAttributes.end(), [&](const auto& attr) { return attr.group == 1; }) };
        if (it != vertexAttributes.end()) {
            desc.inputAssembly.vertexBindingsCount = 2;
            desc.inputAssembly.vertexBindings[1].slot = InputSlotClass::PerInstance;
            desc.inputAssembly.vertexBindings[1].dataStride = sizeof(MaterialVertexInstance);
        }

        pipelines_[mask] = state.device.CreateGraphicsPipeline(desc);
    }
}

void Pipeline::Destroy(GraphicsState& state) {
    for (auto&& [mask, pipeline] : pipelines_) {
        if (pipeline) {
            state.device.DestroyGraphicsPipeline(pipeline);
            pipeline = {};
        }
    }
    pipelines_.clear();
    shader_ = {};
}

gfxapi::GraphicsPipelineHandle Pipeline::GetPipeline(u32 variantMask) const {
    return pipelines_.empty() ? GraphicsPipelineHandle{} : pipelines_.at(VariantMask(variantMask));
}

} // namespace ugine