#include "EditorScene.h"
#include "EditorContext.h"

#include <gfxapi/CommandList.h>
#include <ugine/Profile.h>
#include <ugine/engine/gfx/GraphicsScene.h>

#include <glm/gtx/transform.hpp>

namespace ugine::ed {

struct EditorViewportRenderData {
    gfxapi::Extent2D extent{};
    gfxapi::TextureHandle cameraRtv{};
    gfxapi::TextureHandle cameraDepthBuffer{};
    gfxapi::TextureHandleUnique rtv{};
    glm::mat4 viewProjection{};
};

EditorScene::EditorScene(Engine& engine, World& world, EditorContext& context, IAllocator& allocator)
    : WorldScene{ engine, world }
    , context_{ context }
    , gfxState_{ engine.GetState<GraphicsState>() } {

    // TODO: Single instance.
    using namespace gfxapi;

    renderPass_ = gfxState_->device.CreateRenderPassUnique(RenderPassDesc{
        .name = "EditorViewportPass",
        .colorAttachmentCount = 1,
        .colorAttachments = { 
            RenderPassAttachmentDesc{
                .format = Format::R8G8B8A8_Unorm,
                .loadOp = AttachmentLoadOp::Load,
                .storeOp = AttachmentStoreOp::Store,
                .initialLayout = TextureLayout::ReadOnly,
                .finalLayout = TextureLayout::ReadOnly,
            },
        },
        .depthAttachment = RenderPassAttachmentDesc {
            .format = gfxState_->device.SupportedDepthStencilFormat(),
            .loadOp = AttachmentLoadOp::Load,
            .storeOp = AttachmentStoreOp::Store,
            .initialLayout = TextureLayout::ReadOnly,
            .finalLayout = TextureLayout::ReadOnly,
        }
    });

    outlineRenderPass_ = gfxState_->device.CreateRenderPassUnique(RenderPassDesc{
        .name = "OutlinePass",
        .colorAttachmentCount = 1,
        .colorAttachments = { 
            RenderPassAttachmentDesc{
                .format = Format::R8G8B8A8_Unorm,
                .loadOp = AttachmentLoadOp::Clear,
                .storeOp = AttachmentStoreOp::Store,
                .initialLayout = TextureLayout::ReadOnly,
                .finalLayout = TextureLayout::ReadOnly,
            },
        },
    });

    outlinePSO_ = gfxState_->CreateOutlinePSO(*outlineRenderPass_);

    debugRenderer_ = DebugRenderer{ *gfxState_, *renderPass_ };
}

EditorScene::~EditorScene() {
    auto& r{ world_.Registry() };

    r.clear<EditorViewportRenderData>();
}

void EditorScene::Update() {
    auto gfxScene{ world_.GetScene<GraphicsScene>() };

    debugRenderer_.Clear();

    for (auto&& [ent, cameraComponent, renderData] : world_.Registry().view<CameraComponent, EditorViewportRenderData>().each()) {
        auto go{ world_.Get(ent) };

        const auto proj{ ProjectionMatrix(cameraComponent) };
        renderData.viewProjection = proj * go.ViewMatrix();
        renderData.cameraRtv = gfxScene->GetCameraRtv(go);
        renderData.cameraDepthBuffer = gfxScene->GetCameraDepthBuffer(go);

        const gfxapi::Extent2D extent{ cameraComponent.width, cameraComponent.height };
        if (renderData.extent != extent) {
            renderData.extent = extent;

            using namespace gfxapi;

            renderData.rtv = gfxState_->device.CreateTextureUnique(
                TextureDesc{
                    .name = "ViewportEditorOverlayRTV",
                    .extent = extent,
                    .format = Format::R8G8B8A8_Unorm,
                    .usage = TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled,
                },
                TextureLayout::ReadOnly);
        }
    }
}

void EditorScene::AddEditorData(GameObject& go) {
    go.GetOrCreateComponent<EditorViewportRenderData>();
}

gfxapi::TextureHandle EditorScene::GetCameraRtv(const GameObject& go) const {
    auto& viewport{ go.Component<EditorViewportRenderData>() };
    return *viewport.rtv;
}

void EditorScene::Render(gfxapi::CommandList& cmd) {
    using namespace gfxapi;

    UGINE_GPU_EVENT_C(cmd, label, "Editor Viewport Overlay", 1, 0.5f, 0);

    for (auto&& [ent, renderData] : world_.Registry().view<EditorViewportRenderData>().each()) {
        RenderOutline(cmd, renderData);
        RenderSelected(cmd, renderData);
    }
}

void EditorScene::RenderOutline(gfxapi::CommandList& cmd, const EditorViewportRenderData& renderData) {
    const std::array<gfxapi::ClearValue, 1> clearValue = {
        gfxapi::ClearValue::Color(0, 0, 0, 0),
    };

    auto fb{ gfxState_->GetFramebuffer(gfxapi::DynamicFramebuffer{
        .renderPass = *outlineRenderPass_,
        .extent = renderData.extent,
        .colorAttachmentCount = 1,
        .colorAttachments = { *renderData.rtv },
    }) };

    cmd.BeginRenderPass(fb, gfxapi::Rect2D::FromExtent(renderData.extent), u32(clearValue.size()), clearValue.data());

    auto viewport{ gfxapi::Viewport::FromExtent(renderData.extent) };
    cmd.SetViewport(gfxapi::Viewport{ viewport.x, viewport.y, viewport.width, viewport.height });
    cmd.SetScissor(gfxapi::Rect2D::FromExtent(renderData.extent));

    // TODO:
    struct Params {
        glm::vec3 color;
        uint32_t mask{};
    } params = {
        glm::vec3{ 1.0f, 0.5f + 0.5f * cos(static_cast<float>(2 * context_.Engine().Seconds())), 0.0f },
        0xff,
    };

    cmd.BindPipeline(*outlinePSO_);
    cmd.BindImage(0, 0, renderData.cameraDepthBuffer, gfxapi::TextureAspectFlags::Stencil);
    cmd.PushConstants(gfxapi::ShaderStage::FragmentShader, 0, params);
    cmd.Draw(3, 1, 0, 0);

    cmd.EndRenderPass();
}

void EditorScene::RenderSelected(gfxapi::CommandList& cmd, const EditorViewportRenderData& renderData) {
    const std::array<gfxapi::ClearValue, 2> clearValue = {
        gfxapi::ClearValue::Color(0, 0, 0, 0),
        gfxapi::ClearValue::DepthStencil(1.0f, 0),
    };

    auto fb{ gfxState_->GetFramebuffer(gfxapi::DynamicFramebuffer{
        .renderPass = *renderPass_,
        .extent = renderData.extent,
        .colorAttachmentCount = 1,
        .colorAttachments = { *renderData.rtv },
        .depth = renderData.cameraDepthBuffer,
    }) };

    auto gfxScene{ world_.GetScene<GraphicsScene>() };
    UGINE_ASSERT(gfxScene);

    cmd.BeginRenderPass(fb, gfxapi::Rect2D::FromExtent(renderData.extent), u32(clearValue.size()), clearValue.data());

    auto viewport{ gfxapi::Viewport::FromExtent(renderData.extent) };
    cmd.SetViewport(gfxapi::Viewport{ viewport.x, viewport.height, viewport.width, -(viewport.y + viewport.height) });
    cmd.SetScissor(gfxapi::Rect2D::FromExtent(renderData.extent));

    auto selected{ context_.SelectedGO() };
    if (selected) {
        const auto mvp{ renderData.viewProjection * selected.GlobalTransformation().Matrix() };

        if (selected.Has<CameraComponent>()) {
            const auto& camera{ selected.Component<CameraComponent>() };

            const glm::vec4 color{ 1, 1, 1, 1 };            
            debugRenderer_.RenderFrustum(cmd, camera.vFovDeg, f32(camera.width) / f32(camera.height), camera.zNear, camera.zFar, color, mvp);
        }

        if (selected.Has<LightComponent>()) {
            const auto& light{ selected.Component<LightComponent>() };

            switch (light.type) {
            case LightComponent::Type::Directional:
                debugRenderer_.RenderDirection(cmd, glm::vec4{ light.color.r, light.color.g, light.color.b, 1.0f }, mvp);
                break;
            case LightComponent::Type::Point:
                debugRenderer_.RenderIcosphere(cmd, glm::vec4{ light.color.r, light.color.g, light.color.b, 1.0f }, mvp * glm::scale(glm::vec3{ light.range }));
                break;
            case LightComponent::Type::Spot:
                {
                    const float tan{ 2 * tanf(glm::radians(light.spotAngleDeg)) };
                    const auto rotate{ glm::mat4{ glm::angleAxis(glm::pi<float>() / 2.0f, glm::vec3{ 1, 0, 0 }) } };

                    debugRenderer_.RenderCone(cmd, glm::vec4{ light.color.r, light.color.g, light.color.b, 1.0f },
                        mvp * rotate * glm::scale(glm::vec3{ 2 * tan, light.range, 2 * tan }));
                }
                break;
            }

            // TODO: Shadow camera frustum debug.
            if (light.generatesShadows) {
                const auto& invViewProj{ gfxScene->GetLightCameraView(selected)  * gfxScene->GetLightCameraProjection(selected) };

                std::array<glm::vec3, 8> vertices = {
                    invViewProj * glm::vec4{ -1, -1, -1, 1 },
                    invViewProj * glm::vec4{ 1, -1, -1, 1 },
                    invViewProj * glm::vec4{ 1, 1, -1, 1 },
                    invViewProj * glm::vec4{ -1, 1, -1, 1 },
                    invViewProj * glm::vec4{ -1, -1, 1, 1 },
                    invViewProj * glm::vec4{ 1, -1, 1, 1 },
                    invViewProj * glm::vec4{ 1, 1, 1, 1 },
                    invViewProj * glm::vec4{ -1, 1, 1, 1 },
                };

                debugRenderer_.AddCuboid(vertices.data(), glm::vec4{ 1, 1, 0, 1 });
            }
        }
    }

    //if (DebugHit.Get<bool>() && hit_) {
    //    renderer.AddSphere(hit_->point, 0.1f, glm::vec3{ 1, 1, 0 });
    //    renderer.AddTriangle(hit_->triangle[0], hit_->triangle[1], hit_->triangle[2], glm::vec3{ 1, 0, 1 });
    //}

    //if (DebugRay.Get<bool>()) {
    //    renderer.AddLine(ray_.origin, ray_.dir * 100.0f, glm::vec3{ 0, 0, 1 });
    //}

    debugRenderer_.Render(cmd, renderData.viewProjection);

    cmd.EndRenderPass();
}

} // namespace ugine::ed