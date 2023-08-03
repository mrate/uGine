#include "TestThreading.h"

#include <ugine/Profile.h>

#include <shaders/triangle_fs_hlsl.h>
#include <shaders/triangle_vs_hlsl.h>

#include <iostream>

using namespace ugine;
using namespace ugine::gfxapi;

class Renderer {
public:
    Renderer(Device* device, Swapchain* swapchain, u32 width, u32 height)
        : device_{ device }
        , swapchain_{ swapchain }
        , width_{ width }
        , height_{ height } {
        //
        // Renderpass.
        //
        renderpass_ = device_->CreateRenderPassUnique(RenderPassDesc{
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
        });

        //
        // Frame buffers.
        //
        swapchainFb_.resize(swapchain_->GetCount());
        for (u32 i{}; i < swapchain_->GetCount(); ++i) {
            swapchainFb_[i] = device_->CreateFramebufferUnique(FramebufferDesc{
                .name = "SwapchainFB",
                .width = width_,
                .height = height_,
                .renderPass = *renderpass_,
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

        indexBuffer_ = device_->CreateIndexBufferUnique(triangleIndices);
        //device_->SetName(*indexBuffer, "Scene index buffer");

        const std::vector<Vertex> triangleVertices{
            Vertex{ glm::vec3{ 0, 0.5f, 0 }, glm::vec3{ 1, 0, 0 } },
            Vertex{ glm::vec3{ 0.5f, -0.5f, 0 }, glm::vec3{ 0, 1, 0 } },
            Vertex{ glm::vec3{ -0.5f, -0.5f, 0 }, glm::vec3{ 0, 0, 1 } },
        };

        vertexBuffer_ = device_->CreateVertexBufferUnique(triangleVertices);
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

        pso_ = device_->CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
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
            .renderPass = *renderpass_,
        }) ;

        renderThread_ = std::thread([this] { RenderThread(); });
    }

    ~Renderer() {
        NextFrame(true);

        renderThread_.join();
    }

    void Render() {
        Submit([this](CommandList* cmd) {
            // Render.
            auto frameFb{ *swapchainFb_[swapchain_->GetIndex()] };

            const Viewport viewport{ 0, 0, float(width_), float(height_) };
            const Rect2D scissor{ 0, 0, width_, height_ };
            const auto clearValue{ ClearValue::Color(0, 0, 0, 1) };

            {
                cmd->BeginRenderPass(frameFb, scissor, 1, &clearValue);

                cmd->SetViewport(viewport);
                cmd->SetScissor(scissor);

                cmd->BindPipeline(*pso_);
                cmd->BindVertexBuffer(*vertexBuffer_, 0);
                cmd->BindIndexBuffer(*indexBuffer_, 0, IndexType::Uint16);

                cmd->DrawIndexed(3, 1, 0, 0, 0);

                cmd->EndRenderPass();
            }
        });
    }

    void NextFrame(bool exit = false) {
        {
            std::unique_lock lock{ frameMutex_ };
            exit_ = exit;
            newFrameReady_ = true;

            //std::cout << "[U] NextFrame\n";
            //std::cout << "[U]  - newFrameReady = " << newFrameReady_ << "\n";
            //std::cout << "[U]  - submitReady   = " << submitReady_ << "\n";
        }

        nextFrameCV_.notify_all();
    }

    void WaitSubmit() {
        std::unique_lock lock{ frameMutex_ };
        //std::cout << "[U] WaitSubmit\n";
        //std::cout << "[U]  - newFrameReady = " << newFrameReady_ << "\n";
        //std::cout << "[U]  - submitReady   = " << submitReady_ << "\n";
        submitReadyCV_.wait(lock, [this] { return submitReady_; });
        submitReady_ = false;
    }

private:
    bool WaitFrameStart() {
        PROFILE_EVENT_N("WaitFrameStart");

        std::unique_lock lock{ frameMutex_ };

        //std::cout << "[R] WaitFrameStart\n";
        //std::cout << "[R]  - newFrameReady = " << newFrameReady_ << "\n";
        //std::cout << "[R]  - submitReady   = " << submitReady_ << "\n";

        nextFrameCV_.wait(lock, [this] { return exit_ || newFrameReady_; });
        newFrameReady_ = false;
        renderingQueue_ = 1 - renderingQueue_;

        return !exit_;
    }

    void RenderThread() {
        PROFILE_THREAD("RenderThread");

        while (!exit_) {
            if (!WaitFrameStart()) {
                break;
            }

            //std::cout << "Render TICK\n";
            RenderQueue();

            RenderDone();
        }
    }

    void RenderDone() {
        PROFILE_EVENT_N("RenderDone");

        {
            std::scoped_lock lock{ frameMutex_ };
            submitReady_ = true;

            //std::cout << "[R] RenderDone\n";
            //std::cout << "[R]  - newFrameReady = " << newFrameReady_ << "\n";
            //std::cout << "[R]  - submitReady   = " << submitReady_ << "\n";
        }

        submitReadyCV_.notify_all();
    }

    void RenderQueue() {
        PROFILE_EVENT_N("RenderQueue");

        auto cmd{ device_->BeginCommandList() };

        auto& queue{ queue_[RenderingQueue()] };

        for (auto& command : queue) {
            command(cmd);
        }

        device_->SubmitCommandLists();

        queue.clear();
    }

    template <typename T> void Submit(T&& t) { queue_[SubmitQueue()].push_back(t); }

    std::condition_variable nextFrameCV_;
    std::condition_variable submitReadyCV_;

    std::mutex frameMutex_;

    bool exit_{};
    bool newFrameReady_{};
    bool submitReady_{};

    Device* device_{};
    Swapchain* swapchain_{};
    u32 width_{};
    u32 height_{};

    std::vector<FramebufferHandleUnique> swapchainFb_;

    RenderPassHandleUnique renderpass_;
    BufferHandleUnique vertexBuffer_;
    BufferHandleUnique indexBuffer_;
    GraphicsPipelineHandleUnique pso_;

    std::thread renderThread_;

    std::vector<std::function<void(CommandList* cmd)>> queue_[2];
    int renderingQueue_{};

    int RenderingQueue() const { return renderingQueue_; }
    int SubmitQueue() const { return 1 - renderingQueue_; }
};

void TestThreading::Run() {
    Renderer renderer{ device_.Get(), swapchain_, width_, height_ };

    do {
        Update();

        {
            PROFILE_EVENT_N("Render");

            renderer.Render();
        }

        {
            PROFILE_EVENT_N("NewFrame");

            renderer.NextFrame();
        }

        {
            PROFILE_EVENT_N("WaitSubmit");

            renderer.WaitSubmit();
        }

        //std::cout << "Main tick\n";

        swapchain_->Present();
    } while (!quit_);
}
