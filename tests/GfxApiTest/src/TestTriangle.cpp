#include "TestTriangle.h"

#include <shaders/triangle_fs_hlsl.h>
#include <shaders/triangle_vs_hlsl.h>

using namespace ugine;
using namespace ugine::gfxapi;

void TestTriangle::Run() {
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
        glm::vec3 color;
    };

    const std::vector<uint16_t> triangleIndices{ 0, 2, 1 };

    const auto indexBuffer{ device_->CreateIndexBufferUnique(triangleIndices) };
    //device_->SetName(*indexBuffer, "Scene index buffer");

    const std::vector<Vertex> triangleVertices{
        Vertex{ glm::vec3{ 0, 0.5f, 0 }, glm::vec3{ 1, 0, 0 } },
        Vertex{ glm::vec3{ 0.5f, -0.5f, 0 }, glm::vec3{ 0, 1, 0 } },
        Vertex{ glm::vec3{ -0.5f, -0.5f, 0 }, glm::vec3{ 0, 0, 1 } },
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
            offsetof(Vertex, color),
            Format::R32G32B32_Float,
            InputSlotClass::PerVertex,
        },
    };

    const auto pso{ device_->CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
            .name = "TrianglePSO",
            .vertexShader = CompiledShader {
                    .name = "fullscreen_vs",
                    .entryPoint = "vs",
                    .data = triangle_vs.data,
                    .size = triangle_vs.size,
                },
                .fragmentShader = CompiledShader {
                    .name = "fullscreen_fs",
                    .entryPoint = "ps",
                    .data = triangle_fs.data,
                    .size = triangle_fs.size,
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

    do {
        Update();

        auto cmd{ device_->BeginCommandList() };

        // Render.
        auto frameFb{ *swapchainFb[swapchain_->GetIndex()] };

        {
            cmd->BeginRenderPass(frameFb, scissor, 1, &clearValue);

            cmd->SetViewport(viewport);
            cmd->SetScissor(scissor);

            cmd->BindPipeline(*pso);
            cmd->BindVertexBuffer(*vertexBuffer, 0);
            cmd->BindIndexBuffer(*indexBuffer, 0, IndexType::Uint16);

            cmd->DrawIndexed(static_cast<u32>(triangleIndices.size()), 1, 0, 0, 0);

            cmd->EndRenderPass();
        }

        device_->SubmitCommandLists();
        swapchain_->Present();

    } while (!quit_);
}
