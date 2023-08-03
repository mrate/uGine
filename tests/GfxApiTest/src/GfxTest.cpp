//#include "GfxTest.h"
//
//#include <imgui.h>
//
//using namespace ugine::gfxapi;
//
//void GfxTest::InitImGui() {
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO();
//    (void)io;
//
//    // Fonts.
//    auto& io{ ImGui::GetIO() };
//#ifdef IMGUI_DOCKING
//    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//#endif
//    io.ConfigWindowsMoveFromTitleBarOnly = true;
//
//    fontTexture_ = [&] {
//        unsigned char* fontData{};
//        int texWidth{}, texHeight{};
//        io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
//
//        SubresourceData data{
//            .data = fontData,
//            .size = u64(texWidth * texHeight * 4),
//            .pitch = u32(texWidth * 4),
//        };
//
//        return state->device.CreateTextureUnique(
//            TextureDesc{
//                .name = "ImGuiFont",
//                .extent = Extent2D{ u32(texWidth), u32(texHeight) },
//                .format = Format::R8G8B8A8_Unorm,
//                .usage = TextureUsageFlags::Sampled,
//            },
//            TextureLayout::ReadOnly, data);
//    }();
//
//    fontTextureIndex_ = state->device.GetTextureBindlessIndex(*fontTexture_, TextureAspectFlags::Color);
//
//    fontSampler_ = state->device.CreateSamplerUnique(Sampler("ImGuiFont", TextureAddressMode::Clamp, Filter::Nearest, Filter::Nearest));
//    fontSamplerIndex_ = state->device.GetSamplerBindlessIndex(*fontSampler_);
//
//    imageSampler_ = state->device.CreateSamplerUnique(Sampler("ImGuiImage", TextureAddressMode::Clamp, Filter::Linear, Filter::Linear));
//    imageSamplerIndex_ = state->device.GetSamplerBindlessIndex(*imageSampler_);
//
//    renderpass_ = state->device.CreateRenderPassUnique(
//        RenderPassDesc{
//            .name = "ImGuiPass",
//            .colorAttachmentCount = 1,
//            .colorAttachments = {
//                RenderPassAttachmentDesc{
//                    .format = Format::R8G8B8A8_Unorm,
//                    .finalLayout = TextureLayout::ReadOnly,
//                },
//            },
//            });
//
//    // Pipeline.
//    pipeline_ = [&] {
//        //    // Pipeline cache
//        //    pipelineCache_ = gfx.Device().createPipelineCacheUnique({});
//        const CompiledShader vs{
//            .name = ui_vs.name,
//            .entryPoint = "main",
//            .data = ui_vs.data,
//            .size = ui_vs.size,
//        };
//
//        const CompiledShader fs{
//            .name = ui_fs.name,
//            .entryPoint = "main",
//            .data = ui_fs.data,
//            .size = ui_fs.size,
//        };
//
//        VertexAttribute vertexAttributes[] = {
//            VertexAttribute{
//                0,
//                0,
//                offsetof(ImDrawVert, pos),
//                Format::R32G32B32_Float,
//                InputSlotClass::PerVertex,
//            },
//            VertexAttribute{
//                0,
//                1,
//                offsetof(ImDrawVert, uv),
//                Format::R32G32B32_Float,
//                InputSlotClass::PerVertex,
//            },
//            VertexAttribute{
//                0,
//                2,
//                offsetof(ImDrawVert, col),
//                Format::R8G8B8A8_Unorm,
//                InputSlotClass::PerVertex,
//            },
//        };
//
//        GraphicsPipelineDesc desc{
//            .name = "ImGui",
//            .vertexShader = vs,
//            .fragmentShader = fs,
//            .rasterizerState = {
//                .frontCCW = true,
//                .cullMode = CullMode::None,
//            },
//            .blendState = {
//                .rtv = { BlendRenderTarget {
//                    .enable = true,
//                    .srcBlend = Blend::SrcAlpha,
//                    .dstBlend = Blend::OneMinusSrcAlpha,
//                    .blendOp = BlendOp::Add,
//                    .srcAlphaBlend = Blend::One,
//                    .dstAlphaBlend = Blend::One,
//                    .alphaBlendOp = BlendOp::Add,
//                } },
//            },
//            .depthStencilState = {
//                .depthTestEnable = false,
//                .depthWriteEnable = false,
//            },
//            .inputAssembly = {
//                .primitiveTopology = PrimitiveTopology::TriangleList,
//                .vertexAttributes = vertexAttributes,
//                .vertexAttributesCount = 3,
//                .vertexBindingsCount = 1,
//                .vertexBindings = {
//                    VertexBindings {
//                        .dataStride = sizeof(ImDrawVert),
//                        .slot = InputSlotClass::PerVertex,
//                    },
//                },
//            },
//            .renderPass = *renderpass_,
//        };
//
//        return state->device.CreateGraphicsPipelineUnique(desc);
//    }();
//
//    io.Fonts->TexID = ImGuiTexture(fontTextureIndex_, 0);
//
//    ImGui_ImplVulkan_InitInfo info{};
//    info.CheckVkResultFn = CheckVkResult;
//    state->device.Fill(info);
//    VkRenderPass renderpass{ (VkRenderPass)state->device.GetNativeRenderPass(*renderpass_) };
//    ImGui_ImplVulkan_Init(&info, renderpass);
//}
//
//void GfxTest::DestroyImGui() {
//    ImGui_ImplVulkan_Shutdown();
//}
//
//void GfxTest::NewFrameImGui() {
//    ImGui_ImplVulkan_NewFrame();
//    ImGui::NewFrame();
//}
//
//void GfxTest::RenderImGui() {
//    ImGui::Render();
//
//    auto& io{ ImGui::GetIO() };
//
//    // Update vertex / index data.
//    auto imDrawData{ ImGui::GetDrawData() };
//    const auto vertexBufferSize{ imDrawData->TotalVtxCount * sizeof(ImDrawVert) };
//    const auto indexBufferSize{ imDrawData->TotalIdxCount * sizeof(ImDrawIdx) };
//
//    if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
//        return;
//    }
//
//    shaders::UiParams params{
//        .scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y),
//        .translate = glm::vec2(-1.0f),
//        .flags = 0,
//    };
//
//    // Upload data
//    auto cmd{ device_->BeginCommandList() };
//    auto vertexBuffer{ cmd->AllocateGPU(vertexBufferSize) };
//    auto indexBuffer{ cmd->AllocateGPU(indexBufferSize) };
//
//    { // Generate vertex + index buffer.
//        auto vtxDst{ reinterpret_cast<ImDrawVert*>(vertexBuffer.mapped) };
//        auto idxDst{ reinterpret_cast<ImDrawIdx*>(indexBuffer.mapped) };
//        for (int n{}; n < imDrawData->CmdListsCount; ++n) {
//            const ImDrawList* cmd_list{ imDrawData->CmdLists[n] };
//            memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
//            memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
//            vtxDst += cmd_list->VtxBuffer.Size;
//            idxDst += cmd_list->IdxBuffer.Size;
//        }
//    }
//
//    // Render commands
//    renderTarget_
//        = state->renderTargetCache.Get(state->frameNumber, extent_, Format::R8G8B8A8_Unorm, TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled);
//
//    const ClearValue clearValue{ ClearValue::Color(0, 0, 0, 0) };
//
//    auto fb{ state->framebufferCache.Get(state->frameNumber,
//        DynamicFramebuffer{
//            .renderPass = *renderpass_,
//            .extent = extent_,
//            .colorAttachmentCount = 1,
//            .colorAttachments = { { renderTarget_ } },
//        }) };
//
//    cmd->BeginRenderPass(fb, Rect2D::FromExtent(extent_), 1, &clearValue);
//
//    i32 vertexOffset{};
//    i32 indexOffset{};
//    if (imDrawData->CmdListsCount > 0 && ImGui::GetIO().DisplaySize.x && ImGui::GetIO().DisplaySize.y) {
//        const Viewport viewport{ 0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y };
//        cmd->BindPipeline(*pipeline_);
//        cmd->SetViewport(viewport);
//
//        cmd->BindVertexBuffer(vertexBuffer, 0);
//        cmd->BindIndexBuffer(indexBuffer, 0, IndexType::Uint16);
//
//        for (i32 i{}; i < imDrawData->CmdListsCount; i++) {
//            const ImDrawList* cmd_list{ imDrawData->CmdLists[i] };
//
//            for (i32 j{}; j < cmd_list->CmdBuffer.Size; j++) {
//                const ImDrawCmd* pcmd{ &cmd_list->CmdBuffer[j] };
//
//                const Rect2D scissor{ std::max(i32(pcmd->ClipRect.x), 0), std::max(i32(pcmd->ClipRect.y), 0), u32(pcmd->ClipRect.z - pcmd->ClipRect.x),
//                    u32(pcmd->ClipRect.w - pcmd->ClipRect.y) };
//
//                const auto textureSampler{ reinterpret_cast<i64>(pcmd->TextureId) };
//
//                const i32 textureIndex{ textureSampler & 0xffffffff };
//                if (textureIndex == gfxapi::BindlessInvalid) {
//                    continue;
//                }
//
//                const u32 flags{ (u64(textureSampler) >> 32) & 0xffffffff };
//                const auto samplerIndex{ textureIndex == fontTextureIndex_ ? fontSamplerIndex_ : imageSamplerIndex_ };
//
//                params.textureSampler = glm::uvec2{ textureIndex, samplerIndex == BindlessInvalid ? imageSamplerIndex_ : samplerIndex };
//                params.flags = flags;
//
//                cmd->SetScissor(scissor);
//                cmd->PushConstants(ShaderStage::VertexShader | ShaderStage::FragmentShader, 0, params);
//                cmd->DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + indexOffset, pcmd->VtxOffset + vertexOffset, 0);
//            }
//            indexOffset += cmd_list->IdxBuffer.Size;
//            vertexOffset += cmd_list->VtxBuffer.Size;
//        }
//    }
//
//    cmd->EndRenderPass();
//}
