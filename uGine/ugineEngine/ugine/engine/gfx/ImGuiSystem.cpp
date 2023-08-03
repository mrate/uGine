#include "ImGuiSystem.h"
#include "GraphicsState.h"
#include "ImGui.h"

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/shaders/Shader_UI.h>

#include <gfxapi/Device.h>
#include <gfxapi/Initializers.h>
#include <gfxapi/spirv/SpirvCompiler.h>

#include <ugine/File.h>
#include <ugine/Profile.h>
#include <ugine/Ugine.h>

#include <backends/imgui_impl_vulkan.h>

#include <glm/glm.hpp>

#include <filesystem>

// Generated.

#include <ugineTools/Embed.h>

#include <shaders/ui_fs_hlsl.h>
#include <shaders/ui_vs_hlsl.h>

#include <fonts/roboto_font.h>
// https://pictogrammers.com/library/mdi/
#include <fonts/md_font.h>

using namespace ugine::gfxapi;

#ifndef IMGUI_USE_WCHAR32
#error Need 32-bit WChar for Material Design font. Enable IMGUI_USE_WCHAR32 define in imconfig.h
#endif

namespace ugine {

void CheckVkResult(VkResult err) {
    if (err != VK_SUCCESS) {
        UGINE_ERROR("ImGui vulkan error: {}", static_cast<int>(err));
    }
}

ImGuiSystem::ImGuiSystem(Engine& engine)
    : System{ engine } {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    Init();
}

ImGuiSystem::~ImGuiSystem() {
    auto state{ GetEngine().GetState<GraphicsState>() };
    UGINE_ASSERT(state);
    state->device.WaitIdle();

    ImGui_ImplVulkan_Shutdown();
}

void ImGuiSystem::EarlyUpdate() {
    ImGui_ImplVulkan_NewFrame();

    ImGui::NewFrame();
}

void ImGuiSystem::LateUpdate() {
    PROFILE_EVENT();

    visible_ = false;

    auto state{ GetEngine().GetState<GraphicsState>() };

    ImGui::Render();

    auto& io{ ImGui::GetIO() };

    // Update vertex / index data.
    auto imDrawData{ ImGui::GetDrawData() };
    const auto vertexBufferSize{ imDrawData->TotalVtxCount * sizeof(ImDrawVert) };
    const auto indexBufferSize{ imDrawData->TotalIdxCount * sizeof(ImDrawIdx) };

    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
        return;
    }

    shaders::UiParams params{
        .scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y),
        .translate = glm::vec2(-1.0f),
        .flags = 0,
    };

    // Upload data
    auto cmd{ state->device.BeginCommandList() };

    UGINE_GPU_EVENT_C(*cmd, label, "UI", 0.25f, 0.75f, 1);

    auto vertexBuffer{ cmd->AllocateGPU(vertexBufferSize) };
    auto indexBuffer{ cmd->AllocateGPU(indexBufferSize) };

    { // Generate vertex + index buffer.
        auto vtxDst{ reinterpret_cast<ImDrawVert*>(vertexBuffer.mapped) };
        auto idxDst{ reinterpret_cast<ImDrawIdx*>(indexBuffer.mapped) };
        for (int n{}; n < imDrawData->CmdListsCount; ++n) {
            const ImDrawList* cmd_list{ imDrawData->CmdLists[n] };
            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmd_list->VtxBuffer.Size;
            idxDst += cmd_list->IdxBuffer.Size;
        }
    }

    // Render commands
    renderTarget_ = state->GetRtv(extent_, Format::R8G8B8A8_Unorm, TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled, "ImGuiRenderTarget");

    const ClearValue clearValue{ ClearValue::Color(0, 0, 0, 0) };

    auto fb{ state->GetFramebuffer(DynamicFramebuffer{
        .renderPass = *renderpass_,
        .extent = extent_,
        .colorAttachmentCount = 1,
        .colorAttachments = { { renderTarget_ } },
    }) };

    cmd->BeginRenderPass(fb, Rect2D::FromExtent(extent_), 1, &clearValue);

    i32 vertexOffset{};
    i32 indexOffset{};
    if (imDrawData->CmdListsCount > 0 && ImGui::GetIO().DisplaySize.x && ImGui::GetIO().DisplaySize.y) {
        const Viewport viewport{ 0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
        cmd->BindPipeline(*pipeline_);
        cmd->SetViewport(viewport);

        cmd->BindVertexBuffer(vertexBuffer, 0);
        cmd->BindIndexBuffer(indexBuffer, 0, IndexType::Uint16);

        for (i32 i{}; i < imDrawData->CmdListsCount; i++) {
            const ImDrawList* cmd_list{ imDrawData->CmdLists[i] };

            for (i32 j{}; j < cmd_list->CmdBuffer.Size; j++) {
                const ImDrawCmd* pcmd{ &cmd_list->CmdBuffer[j] };

                const Rect2D scissor{ std::max(i32(pcmd->ClipRect.x), 0), std::max(i32(pcmd->ClipRect.y), 0), u32(pcmd->ClipRect.z - pcmd->ClipRect.x),
                    u32(pcmd->ClipRect.w - pcmd->ClipRect.y) };

                const auto textureSampler{ reinterpret_cast<i64>(pcmd->TextureId) };

                const i32 textureIndex{ textureSampler & 0xffffffff };
                if (textureIndex == gfxapi::BindlessInvalid) {
                    continue;
                }

                const u32 flags{ (u64(textureSampler) >> 32) & 0xffffffff };
                const auto samplerIndex{ textureIndex == fontTextureIndex_ ? fontSamplerIndex_ : imageSamplerIndex_ };

                params.textureSampler = glm::uvec2{ textureIndex, samplerIndex == BindlessInvalid ? imageSamplerIndex_ : samplerIndex };
                params.flags = flags;

                cmd->SetScissor(scissor);
                cmd->PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, params);
                cmd->DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + indexOffset, pcmd->VtxOffset + vertexOffset, 0);
            }
            indexOffset += cmd_list->IdxBuffer.Size;
            vertexOffset += cmd_list->VtxBuffer.Size;
        }
    }

    cmd->EndRenderPass();

    visible_ = true;
}

bool ImGuiSystem::HandleSystemEvent(const SystemEvent& event) {
    if (event.type == SystemEvent::Type::WindowResized) {
        Resize(event.resize.width, event.resize.height);
    }
    return false;
}

void ImGuiSystem::Init() {
    auto state{ GetEngine().GetState<GraphicsState>() };

    // Fonts.
    auto& io{ ImGui::GetIO() };
#ifdef IMGUI_DOCKING
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    {
        ImFontConfig iconsConfig{};
        iconsConfig.FontDataOwnedByAtlas = false,
        io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(reinterpret_cast<const void*>(roboto_font.data)), int(roboto_font.size), 14.0f, &iconsConfig);
    }

    {
        const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        ImFontConfig iconsConfig{};
        iconsConfig.FontDataOwnedByAtlas = false;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.GlyphMinAdvanceX = 16;

        io.Fonts->AddFontFromMemoryTTF(const_cast<void*>(reinterpret_cast<const void*>(md_font.data)), int(md_font.size), 14.0f, &iconsConfig, iconsRanges);
    }

    fontTexture_ = [&] {
        unsigned char* fontData{};
        int texWidth{}, texHeight{};
        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

        SubresourceData data{
            .data = fontData,
            .size = u64(texWidth * texHeight * 4),
            .pitch = u32(texWidth * 4),
        };

        return state->device.CreateTextureUnique(
            TextureDesc{
                .name = "ImGuiFont",
                .extent = Extent2D{ u32(texWidth), u32(texHeight) },
                .format = Format::R8G8B8A8_Unorm,
                .usage = TextureUsageFlags::Sampled,
            },
            TextureLayout::ReadOnly, data);
    }();

    fontTextureIndex_ = state->device.GetTextureBindlessIndex(*fontTexture_, TextureAspectFlags::Color);

    fontSampler_ = state->device.CreateSamplerUnique(Sampler("ImGuiFont", TextureAddressMode::Clamp, Filter::Nearest, Filter::Nearest));
    fontSamplerIndex_ = state->device.GetSamplerBindlessIndex(*fontSampler_);

    imageSampler_ = state->device.CreateSamplerUnique(Sampler("ImGuiImage", TextureAddressMode::Clamp, Filter::Linear, Filter::Linear));
    imageSamplerIndex_ = state->device.GetSamplerBindlessIndex(*imageSampler_);

    renderpass_ = state->device.CreateRenderPassUnique(
        RenderPassDesc{ 
            .name = "ImGuiPass",
            .colorAttachmentCount = 1,
            .colorAttachments = {
                RenderPassAttachmentDesc{ 
                    .format = Format::R8G8B8A8_Unorm, 
                    .finalLayout = TextureLayout::ReadOnly, 
                },
            },            
            });

    // Pipeline.
    pipeline_ = [&] {
        //    // Pipeline cache
        //    pipelineCache_ = gfx.Device().createPipelineCacheUnique({});
        const CompiledShader vs{
            .name = ui_vs.name,
            .entryPoint = "main",
            .data = ui_vs.data,
            .size = ui_vs.size,
        };

        const CompiledShader fs{
            .name = ui_fs.name,
            .entryPoint = "main",
            .data = ui_fs.data,
            .size = ui_fs.size,
        };

        VertexAttribute vertexAttributes[] = {
            VertexAttribute{
                0,
                0,
                offsetof(ImDrawVert, pos),
                Format::R32G32B32_Float,
                InputSlotClass::PerVertex,
            },
            VertexAttribute{
                0,
                1,
                offsetof(ImDrawVert, uv),
                Format::R32G32B32_Float,
                InputSlotClass::PerVertex,
            },
            VertexAttribute{
                0,
                2,
                offsetof(ImDrawVert, col),
                Format::R8G8B8A8_Unorm,
                InputSlotClass::PerVertex,
            },
        };

        GraphicsPipelineDesc desc{
            .name = "ImGui",
            .vertexShader = vs,
            .fragmentShader = fs, 
            .rasterizerState = {
                .frontCCW = true,
                .cullMode = CullMode::None,
            },
            .blendState = {
                .rtv = { BlendRenderTarget {
                    .enable = true,
                    .srcBlend = Blend::SrcAlpha,
                    .dstBlend = Blend::OneMinusSrcAlpha,
                    .blendOp = BlendOp::Add,
                    .srcAlphaBlend = Blend::One,
                    .dstAlphaBlend = Blend::One,
                    .alphaBlendOp = BlendOp::Add,
                } },
            },
            .depthStencilState = {
                .depthTestEnable = false,
                .depthWriteEnable = false,
            },
            .inputAssembly = {
                .primitiveTopology = PrimitiveTopology::TriangleList,
                .vertexAttributes = vertexAttributes,
                .vertexAttributesCount = 3,
                .vertexBindingsCount = 1,
                .vertexBindings = {
                    VertexBindings {
                        .dataStride = sizeof(ImDrawVert),
                        .slot = InputSlotClass::PerVertex,
                    },
                },
            },
            .renderPass = *renderpass_,
        };

        return state->device.CreateGraphicsPipelineUnique(desc);
    }();

    io.Fonts->TexID = ImGuiTexture(fontTextureIndex_);

    ImGui_ImplVulkan_InitInfo info{};
    info.CheckVkResultFn = CheckVkResult;
    state->device.Fill(info);
    VkRenderPass renderpass{ (VkRenderPass)state->device.GetNativeRenderPass(*renderpass_) };
    ImGui_ImplVulkan_Init(&info, renderpass);
}

void ImGuiSystem::Resize(u32 width, u32 height) {
    extent_ = Extent2D{ width, height };
}

} // namespace ugine