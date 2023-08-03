#include "TestBindless.h"

#include <gfxapi/Initializers.h>

#include <shaders/bindless_fs_hlsl.h>
#include <shaders/bindless_vs_hlsl.h>

#include <chrono>
#include <string>

using namespace ugine;
using namespace ugine::gfxapi;

void TestBindless::Run() {
    //
    // Renderpass.
    //
    auto renderpass {
        device_->CreateRenderPassUnique(RenderPassDesc{
            .colorAttachmentCount = 1, 
            .colorAttachments = RenderPassAttachmentDesc {
                .format = swapchain_->GetFormat(),
                .loadOp = AttachmentLoadOp::Clear,
                .storeOp = AttachmentStoreOp::Store,
                .stencilLoadOp = AttachmentLoadOp::DontCare,
                .stencilStoreOp = AttachmentStoreOp::DontCare,
                .initialLayout = TextureLayout::Undefined,
                .finalLayout = TextureLayout::Present,
            },
        })
    };

    //
    // Frame buffers.
    //
    std::vector<FramebufferHandleUnique> swapchainFb(swapchain_->GetCount());
    for (u32 i{}; i < swapchain_->GetCount(); ++i) {
        swapchainFb[i] = device_->CreateFramebufferUnique(FramebufferDesc{
            .name = "SwapchainFB",
            .width = width_,
            .height = height_,
            .renderPass = *renderpass,
            .colorAttachmentCount = 1,
            .colorAttachments = { swapchain_->GetTexture(i) },
        });
    }

    //
    // Vertex + index buffer, Pipeline.
    //

    struct Vertex {
        glm::vec3 position;
        glm::vec2 uv;
    };

    const std::vector<uint16_t> triangleIndices{ 0, 2, 1 };

    const auto indexBuffer{ device_->CreateIndexBufferUnique(triangleIndices) };
    //device_->SetName(*indexBuffer, "Scene index buffer");

    const std::vector<Vertex> triangleVertices{
        Vertex{ glm::vec3{ 0, 0.5f, 0 }, glm::vec2{ 0, 0 } },
        Vertex{ glm::vec3{ 0.5f, -0.5f, 0 }, glm::vec2{ 0, 1 } },
        Vertex{ glm::vec3{ -0.5f, -0.5f, 0 }, glm::vec2{ 1, 0 } },
    };

    const auto vertexBuffer{ device_->CreateVertexBufferUnique(triangleVertices) };
    //device_->SetName(*vertexBuffer, "Scene vertex buffer");

    VertexAttribute vertexAttributes[2] = {
        VertexAttribute{
            0,
            0,
            offsetof(Vertex, position),
            Format::R32G32B32_Float,
            InputSlotClass::PerVertex,
        },
        VertexAttribute{
            0,
            1,
            offsetof(Vertex, uv),
            Format::R32G32_Float,
            InputSlotClass::PerVertex,
        },
    };

    const auto pso{ device_->CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
            .name = "BindlessPSO",
            .vertexShader = CompiledShader {
                .name = "bindless_vs",
                .entryPoint = "vs",
                .data = bindless_vs.data,
                .size = bindless_vs.size,
            },
            .fragmentShader = CompiledShader {
                .name = "bindless_fs",
                .entryPoint = "ps",
                .data = bindless_fs.data,
                .size = bindless_fs.size,
            },
            .rasterizerState = {
                .frontCCW = true,
                .cullMode = CullMode::None,
                .fillMode = FillMode::Fill,
            },
            .inputAssembly = {
                .vertexAttributes = vertexAttributes,
                .vertexAttributesCount = 2,
                .vertexBindingsCount = 1,
                .vertexBindings = { VertexBindings {
                        .dataStride = sizeof(Vertex),
                    },
                },
            },
            .renderPass = *renderpass,
        }) };

    const Viewport viewport{ 0, 0, float(width_), float(height_) };
    const Rect2D scissor{ 0, 0, width_, height_ };
    const auto clearValue{ ClearValue::Color(0, 0, 0, 1) };

    std::array<TextureHandleUnique, 10> textures;
    for (u32 i{}; i < 10; ++i) {
        const u32 color{ ColorRGBA{ f32(i % 3) / 2.0f, f32(i % 4) / 3.0f, f32(i % 5) / 4.0f, 1 }.ToUint() };

        SubresourceData data{
            .data = &color,
            .size = 4 * sizeof(u32),
            .pitch = 4 * sizeof(u32),
        };

        textures[i] = device_->CreateTextureUnique(
            TextureDesc{
                .name = "Texture" + std::to_string(i),
                .extent = Extent2D{ 1, 1 },
                .format = Format::R8G8B8A8_Unorm,
                .usage = TextureUsageFlags::Sampled,
            },
            TextureLayout::ReadOnly, data);
    }

    auto sampler{ device_->CreateSamplerUnique(Sampler("Sampler", TextureAddressMode::Wrap, Filter::Linear, Filter::Nearest)) };

    using Clock = std::chrono::high_resolution_clock;
    const auto start{ Clock::now() };

    struct BindlessIndex {
        u32 textureIndex{};
        u32 samplerIndex{};
    } bindlessIndex;

    do {
        Update();

        auto cmd{ device_->BeginCommandList() };

        // Render.
        auto frameFb{ *swapchainFb[swapchain_->GetIndex()] };

        bindlessIndex.textureIndex = u32(std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - start).count() % textures.size());
        bindlessIndex.samplerIndex = 0;

        {
            cmd->BeginRenderPass(frameFb, scissor, 1, &clearValue);

            cmd->SetViewport(viewport);
            cmd->SetScissor(scissor);

            cmd->BindPipeline(*pso);
            cmd->BindVertexBuffer(*vertexBuffer, 0);
            cmd->BindIndexBuffer(*indexBuffer, 0, IndexType::Uint16);
            cmd->PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, bindlessIndex);

            cmd->DrawIndexed(static_cast<u32>(triangleIndices.size()), 1, 0, 0, 0);

            cmd->EndRenderPass();
        }

        device_->SubmitCommandLists();
        swapchain_->Present();

    } while (!quit_);
}
