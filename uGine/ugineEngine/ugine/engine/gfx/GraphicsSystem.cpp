#include "GraphicsSystem.h"
#include "Consts.h"
#include "GraphicsScene.h"
#include "GraphicsState.h"

#include "Animation.h"
#include "Material.h"
#include "Model.h"
#include "Texture.h"

#include <ugine/engine/core/ResourceStorage.h>

#include <gfxapi/FramebufferCache.h>
#include <gfxapi/Initializers.h>
#include <gfxapi/spirv/SpirvCompiler.h>

#include <ugine/engine/core/ResourceManager.h>
#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/system/Platform.h>
#include <ugine/engine/world/WorldManager.h>

#include <ugine/File.h>
#include <ugine/Log.h>
#include <ugine/Profile.h>
#include <ugine/Ugine.h>

#include <ugineTools/Embed.h>

#include <shaders/fullscreen_fs_hlsl.h>
#include <shaders/fullscreen_vs_hlsl.h>

#include <vulkan/vulkan.h>

namespace ugine {

GraphicsSystem::GraphicsSystem(Engine& engine)
    : System{ engine }
    , swapchainFB_{ engine.GetAllocator() }
    , layers_{ engine.GetAllocator() } {

    engine.GetWorldManager().Connect<WorldManager::WorldCreatedEvent, &GraphicsSystem::OnWorldCreated>(this);

    const auto [appMajor, appMinor, appFile] = engine.GetParams().appVersion;

    gfxapi::DeviceCreateInfo deviceCI{
        .vulkan = {
            .appName = engine.GetParams().appName.Data(),
            .appVer = {appMajor, appMinor, appFile}, 
            .engineName = "uGine", 
            .engineVer = {UGINE_VERSION_MAJOR, UGINE_VERSION_MINOR, UGINE_VERSION_FILE},
            .maxCommandBuffers = MAX_COMMANDLIST_COUNT,
        },
        .validationLayers = engine.GetParams().debugGraphics,
    };

    engine.GetPlatform().FillDeviceCreateInfo(deviceCI);
    device_ = gfxapi::Device::Create(deviceCI, engine.GetAllocator());

    swapchain_ = device_->GetSwapchain();
    state_ = &GetEngine().AttachState<GraphicsState>(engine, *device_, swapchain_->GetCount());

    CreateSwapchain();

    using namespace ugine::gfxapi;

    // TODO:
    pipeline_ = device_->CreateGraphicsPipelineUnique(GraphicsPipelineDesc {
                .name = "FullscreenPSO",
                .vertexShader = CompiledShader {
                    .name = "fullscreen_vs",
                    .entryPoint = "main",
                    .data = fullscreen_vs.data,
                    .size = fullscreen_vs.size,
                },
                .fragmentShader = CompiledShader {
                    .name = "fullscreen_fs",
                    .entryPoint = "main",
                    .data = fullscreen_fs.data,
                    .size = fullscreen_fs.size,
                },
                .rasterizerState = {
                    .frontCCW = false,
                    .cullMode = CullMode::Back,
                    .fillMode = FillMode::Fill,
                    },
                .blendState = {
                    .rtv = {
                        BlendRenderTarget{ 
                            .enable = true,
                            .srcBlend = Blend::SrcAlpha,
                            .dstBlend = Blend::OneMinusSrcAlpha,
                            .blendOp = BlendOp::Add,
                            .srcAlphaBlend = Blend::One,
                            .dstAlphaBlend = Blend::One,
                            .alphaBlendOp = BlendOp::Add,
                        },
                    }
                },
                .renderPass = *swapchainRenderpass_,
            });
}

GraphicsSystem::~GraphicsSystem() {
    device_->WaitIdle();
    swapchain_ = nullptr;

    // TODO: Order matters (due to dependencies...)
    GetEngine().GetResources().Clear<Model>();
    GetEngine().GetResources().Clear<Material>();
    GetEngine().GetResources().Clear<Animation>();
    GetEngine().GetResources().Clear<Shader>();
    GetEngine().GetResources().Clear<Texture>();

    pipeline_ = {};

    swapchainFB_.Clear();
    swapchainRenderpass_ = {};

    state_ = nullptr;
    GetEngine().DeleteState<GraphicsState>();
    GetEngine().GetWorldManager().Disconnect<WorldManager::WorldCreatedEvent>(this);

    device_ = nullptr;
}

bool GraphicsSystem::HandleSystemEvent(const SystemEvent& event) {
    switch (event.type) {
        using enum SystemEvent::Type;

    case WindowResized: CreateSwapchain(); break;
    }

    return false;
}

void GraphicsSystem::Update() {
    auto& wm{ GetEngine().GetWorldManager() };

    wm.ForEachScene<GraphicsScene>([&](GraphicsScene& scene) {
        auto& world{ scene.GetWorld() };
        if (!world.IsRendering()) {
            return;
        }

        scene.Update();
    });

    auto cmd{ device_->BeginCommandList() };
    wm.ForEachScene<GraphicsScene>([&](GraphicsScene& scene) {
        auto& world{ scene.GetWorld() };
        if (!world.IsRendering()) {
            return;
        }

        scene.Render(*cmd);
    });
}

void GraphicsSystem::Sync() {

    auto cmd{ device_->BeginCommandList() };
    auto clearValue{ gfxapi::ClearValue::Color(0, 0, 0, 1.0f) };

    using namespace ugine::gfxapi;

    {
        UGINE_GPU_EVENT_C(*cmd, label, "Present", 0.75f, 0.75f, 0.75f);

        cmd->BeginRenderPass(*swapchainFB_[swapchain_->GetIndex()], gfxapi::Rect2D::FromExtent(swapchain_->GetExtent()), 1, &clearValue);

        cmd->SetViewport(gfxapi::Viewport::FromExtent(swapchain_->GetExtent()));
        cmd->SetScissor(gfxapi::Rect2D::FromExtent(swapchain_->GetExtent()));
        cmd->BindPipeline(*pipeline_);

        if (state_->mainOutputOverride) {
            cmd->BindImageSampler(0, 0, state_->mainOutputOverride, *state_->samplerWrapLinearNearest);
            cmd->Draw(3, 1, 0, 0);
        }

        for (const auto layer : layers_) {
            if (layer->Visible()) {
                cmd->Barrier(ImageBarrier{
                    .texture = layer->View(),
                    .srcLayout = TextureLayout::ReadOnly,
                    .srcAccess = AccessFlags::ColorAttachmentWrite,
                    .srcStage = PipelineStageFlags::ColorAttachmentOutput,
                    .dstLayout = TextureLayout::ReadOnly,
                    .dstAccess = AccessFlags::ShaderRead,
                    .dstStage = PipelineStageFlags::FragmentShader,
                });

                cmd->BindImageSampler(0, 0, layer->View(), *state_->samplerWrapLinearNearest);
                cmd->Draw(3, 1, 0, 0);
            }
        }

        cmd->EndRenderPass();
    }

    //state_->renderThread.NextFrame();
    //state_->renderThread.WaitSubmit();

    device_->SubmitCommandLists();
    swapchain_->Present();

    ++state_->frameNumber;

    // TODO: Frames?
    if (state_->frameNumber >= 8) {
        state_->framebufferCache.PurgeOlderThen(state_->frameNumber - 8);
        state_->rtvCache.PurgeOlderThen(state_->frameNumber - 8);
    }
}

void GraphicsSystem::CreateSwapchain() {
    using namespace ugine::gfxapi;

    swapchainFB_.Clear();
    swapchain_->Reset();

    swapchainRenderpass_ = device_->CreateRenderPassUnique(gfxapi::RenderPassDesc{
        .name = "SwapchainPass",
        .colorAttachmentCount = 1,
        .colorAttachments = { RenderPassAttachmentDesc{
            .format = swapchain_->GetFormat(),
            .finalLayout = TextureLayout::Present,
        } },
    });

    swapchainFB_.Resize(swapchain_->GetCount());
    for (u32 i{}; i < swapchain_->GetCount(); ++i) {
        swapchainFB_[i] = device_->CreateFramebufferUnique(gfxapi::FramebufferDesc{
            .name = "SwapchainFB",
            .width = swapchain_->GetExtent().width,
            .height = swapchain_->GetExtent().height,
            .renderPass = *swapchainRenderpass_,
            .colorAttachmentCount = 1,
            .colorAttachments = { swapchain_->GetTexture(i) },
        });
    }
}

// Reactive:

void GraphicsSystem::OnWorldCreated(const WorldManager::WorldCreatedEvent& event) {
    event.world->AddScene(MakeUnique<GraphicsScene>(GetEngine().GetAllocator(), GetEngine(), *event.world, *state_, GetEngine().GetAllocator()));
}

} // namespace ugine