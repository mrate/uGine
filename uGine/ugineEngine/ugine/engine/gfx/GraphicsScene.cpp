#include "GraphicsScene.h"

#include <gfxapi/Initializers.h>

#include <ugine/Hash.h>
#include <ugine/Profile.h>

#include <ugine/engine/engine/CVars.h>
#include <ugine/engine/gfx/GraphicsState.h>
#include <ugine/engine/math/Culling.h>
#include <ugine/engine/math/Raycast.h>
#include <ugine/engine/world/Component.h>
#include <ugine/engine/world/World.h>

#include <ugine/engine/gfx/pass/DepthPrePass.h>
#include <ugine/engine/gfx/pass/ForwardPass.h>
#include <ugine/engine/gfx/pass/LightCullingPass.h>
#include <ugine/engine/gfx/pass/ShadowPass.h>
#include <ugine/engine/gfx/pass/SsaoPass.h>
#include <ugine/engine/gfx/pass/TonemappingPass.h>

namespace ugine {

using namespace gfxapi;

namespace {
    auto& DebugAabb{ CVars::Register("Debug AABB", "Enable AABB vizualization", "graphics", CVar::Type::Bool, false) };
    auto& DebugSphere{ CVars::Register("Debug bounding sphere", "Enable bounding sphere vizualization", "graphics", CVar::Type::Bool, false) };

    auto& DebugLightCull{ CVars::Register(
        "Debug light cull", "Debug light culling (0 = off, 1 = opaque, 2 = transparent)", "graphics", CVar::Type::Int, 0, 0, 2) };
    auto& DisableSSAO{ CVars::Register("Disable SSAO", "Disable SSAO rendering", "graphics", CVar::Type::Bool, true) }; // TODO: Fix SSAO.
    auto& DisableDrawSort{ CVars::Register("Disable draw sort", "Disable draw call sorting", "graphics", CVar::Type::Bool, false) };
} // namespace

struct LightShaderData {
    // TODO:
    static constexpr auto page_size{ sizeof(shaders::Light) * 1024 };

    shaders::Light light;
};

struct LightRenderData {
    AABB aabb;
    shaders::Camera camera;

    // Shadow data.
    // TODO: CSM.
    bool isShadowCaster{};
    gfxapi::TextureHandle shadowMap;
    VisibilityList visibilityList;

    ParallelCull cull;
};

struct CameraRenderData {
    gfxapi::Extent2D extent{};
    gfxapi::BufferHandleUnique lightCullFrustums{};
    Camera cCamera; // TODO:
    shaders::Camera camera;
    bool updateFrustums{};

    ParallelCull cull;
    VisibilityList visibilityList;

    gfxapi::Extent2D rtvExtent{};
    gfxapi::TextureHandle renderTarget{};
    gfxapi::TextureHandle depthBuffer{};

    bool customRtv{};
};

struct MeshRenderData {
    ModelInstance modelInstance;
    Sphere boundingShpere{};
    AABB aabb{};
    glm::mat4 modelMatrix;
    glm::mat4 invModelMatrix;
    bool modelReady{};
    bool aabbReady{};
};

struct InstanceRenderData {
    struct PerFrameInstance {
        gfxapi::BufferHandle instanceBuffer{};
        u32 instanceBufferSize{};
    };

    u32 count{};
    u32 updateIndex{};
    Vector<PerFrameInstance> perFrameInstance;
};

struct AnimatorRenderData {
    f64 lastUpdateTimeS{};

    struct PerFrameSkin {
        gfxapi::BufferHandle vertexBuffer{};
        gfxapi::BufferHandle matrixBuffer{};
    };

    ResourceHandle<Animation> animation;
    bool ready{};
    bool syncAnimation{};
    u32 updateIndex{};
    u64 vertexBufferSize{};
    Vector<glm::mat4> boneMatrices{};

    Vector<PerFrameSkin> perFrameSkin;
};

struct SkyRenderData {
    ResourceHandle<Material> material;
};

// Helpers.
struct PendingModelFlag {};
struct PendingAnimationFlag {};

GraphicsScene::GraphicsScene(Engine& engine, World& world, GraphicsState& state, IAllocator& allocator)
    : WorldScene{ engine, world }
    , state_{ state }
    , allocator_{ allocator } // TODO: Remove render pass dependency.
    , debugRenderer_{ state_, state_.GetRenderPass(GraphicsState::RenderPass::ForwardLDR), allocator } {

    for (u32 i{}; i < state_.framesInFlight; ++i) {
        queryPools_.EmplaceBack(state_.device.CreateQueryPoolUnique(QueryPoolDesc{
            .type = QueryType::Timestamp,
            .count = 64,
        }));
    }

    auto& r{ world_.Registry() };

#define ATTACH_COMPONENT(COMP_TYPE, RENDER_TYPE)                                                                                                               \
    do {                                                                                                                                                       \
        r.on_construct<COMP_TYPE>().connect<&GameObjectRegistry::emplace<RENDER_TYPE>>();                                                                      \
        r.on_destroy<COMP_TYPE>().connect<&SafeRemoveComponent<RENDER_TYPE>>();                                                                                \
    } while (0)
#define ATTACH_COMPONENT_C(COMP_TYPE, RENDER_TYPE, CREATE_FUNC)                                                                                                \
    do {                                                                                                                                                       \
        r.on_construct<COMP_TYPE>().connect<&GameObjectRegistry::emplace<RENDER_TYPE>>();                                                                      \
        r.on_destroy<COMP_TYPE>().connect<&SafeRemoveComponent<RENDER_TYPE>>();                                                                                \
        r.on_construct<RENDER_TYPE>().connect<&GraphicsScene::CREATE_FUNC>(this);                                                                              \
    } while (0)
#define ATTACH_COMPONENT_CD(COMP_TYPE, RENDER_TYPE, CREATE_FUNC, DESTROY_FUNC)                                                                                 \
    do {                                                                                                                                                       \
        r.on_construct<COMP_TYPE>().connect<&GameObjectRegistry::emplace<RENDER_TYPE>>();                                                                      \
        r.on_destroy<COMP_TYPE>().connect<&SafeRemoveComponent<RENDER_TYPE>>();                                                                                \
        r.on_construct<RENDER_TYPE>().connect<&GraphicsScene::CREATE_FUNC>(this);                                                                              \
        r.on_destroy<RENDER_TYPE>().connect<&GraphicsScene::DESTROY_FUNC>(this);                                                                               \
    } while (0)

    ATTACH_COMPONENT_C(LightComponent, LightRenderData, LightRenderDataCreated);
    ATTACH_COMPONENT_C(LightComponent, LightShaderData, LightShaderDataCreated);
    ATTACH_COMPONENT_C(CameraComponent, CameraRenderData, CameraRenderDataCreated);
    ATTACH_COMPONENT_CD(MeshComponent, MeshRenderData, MeshCreated, MeshDestroyed);
    ATTACH_COMPONENT_CD(AnimationControllerComponent, AnimatorRenderData, AnimatorRenderDataCreated, AnimatorRenderDataDestroyed);
    ATTACH_COMPONENT_C(SkyComponent, SkyRenderData, SkyCreated);

    r.on_construct<InstanceRenderData>().connect<&GraphicsScene::InstancedRenderDataCreated>(this);
    r.on_destroy<InstanceRenderData>().connect<&GraphicsScene::InstancedRenderDataDestroyed>(this);

    r.on_destroy<MeshRenderData>().connect<&SafeRemoveComponent<PendingModelFlag>>();
    r.on_destroy<MeshRenderData>().connect<&SafeRemoveComponent<InstanceRenderData>>();
    r.on_destroy<AnimatorRenderData>().connect<&SafeRemoveComponent<PendingAnimationFlag>>();

#undef ATTACH_COMPONENT
#undef ATTACH_COMPONENT_C
#undef ATTACH_COMPONENT_CD

    //
    updatedLightTransformations_.connect(world.Registry(), entt::collector.update<TransformationComponent>().where<LightComponent>());
    updatedLights_.connect(world.Registry(), entt::collector.update<LightComponent>());

    updatedCameras_.connect(world.Registry(), entt::collector.update<CameraComponent>());
    translatedCameras_.connect(world.Registry(), entt::collector.update<TransformationComponent>().where<CameraRenderData>());

    updatedMeshes_.connect(world.Registry(), entt::collector.update<MeshComponent>());
    translatedMeshes_.connect(world.Registry(), entt::collector.update<TransformationComponent>().where<MeshRenderData>());

    updatedAnimationControllers_.connect(world.Registry(), entt::collector.update<AnimationControllerComponent>());
}

GraphicsScene::~GraphicsScene() {
    auto& r{ world_.Registry() };

    r.clear<LightShaderData>();
    r.clear<CameraRenderData>();
    r.clear<MeshRenderData>();
    r.clear<AnimatorRenderData>();
    r.clear<SkyRenderData>();
    r.clear<InstanceRenderData>();

    updatedLightTransformations_.disconnect();
    updatedLights_.disconnect();

    translatedCameras_.disconnect();
    updatedCameras_.disconnect();
    updatedMeshes_.disconnect();
    translatedMeshes_.disconnect();
    updatedAnimationControllers_.disconnect();

    r.on_construct<LightComponent>().disconnect(this);
    r.on_destroy<LightComponent>().disconnect(this);
    r.on_construct<CameraComponent>().disconnect(this);
    r.on_destroy<CameraComponent>().disconnect(this);
    r.on_construct<AnimationControllerComponent>().disconnect(this);
    r.on_destroy<AnimationControllerComponent>().disconnect(this);
    r.on_construct<SkyComponent>().disconnect(this);
    r.on_destroy<SkyComponent>().disconnect(this);
    r.on_construct<MeshComponent>().disconnect(this);
    r.on_destroy<MeshComponent>().disconnect(this);

    r.on_construct<LightRenderData>().disconnect();
    r.on_construct<CameraRenderData>().disconnect();
    r.on_construct<SkyRenderData>().disconnect();
    r.on_construct<MeshRenderData>().disconnect();
    r.on_destroy<MeshRenderData>().disconnect();
    r.on_destroy<InstanceRenderData>().disconnect();
    r.on_construct<AnimatorRenderData>().disconnect();
    r.on_destroy<AnimatorRenderData>().disconnect();
}

Camera GraphicsScene::LightCamera(const LightComponent& shadowCaster, const gfxapi::Extent2D& resolution) const {
    // TODO:
    const auto csmLightSize{ 10 };

    switch (shadowCaster.type) {
    case LightComponent::Type::Directional:
        // TODO: Tight around main camera.
        return Camera::Ortho(-csmLightSize, csmLightSize, csmLightSize, -csmLightSize, -100.0f, 100.0f);
        break;
    case LightComponent::Type::Spot:
        return Camera::Perspective(
            2 * shadowCaster.spotAngleDeg, static_cast<f32>(resolution.width), static_cast<f32>(resolution.height), 0.01f, shadowCaster.range);
        break;
    case LightComponent::Type::Point:
        return Camera::Perspective(90.0f, static_cast<f32>(resolution.width), static_cast<f32>(resolution.height), 0.01f, shadowCaster.range);
    default: UGINE_ASSERT(false); return Camera{};
    }
}

gfxapi::TextureHandle GraphicsScene::GetCameraRtv(const GameObject& go) const {
    return go.Component<CameraRenderData>().renderTarget;
}

int GraphicsScene::GetAoTextureIndex(const GameObject& go) const {
    return go.Component<CameraRenderData>().camera.aoTexture;
}

void GraphicsScene::SetCameraRtv(const GameObject& go, gfxapi::TextureHandle output, const gfxapi::Extent2D& extent) {
    auto& renderData{ go.Component<CameraRenderData>() };
    renderData.renderTarget = output;
    renderData.customRtv = true;
    renderData.extent = renderData.rtvExtent = extent;
}

gfxapi::TextureHandle GraphicsScene::GetCameraDepthBuffer(const GameObject& go) const {
    return go.Component<CameraRenderData>().depthBuffer;
}

gfxapi::TextureHandle GraphicsScene::GetShadowMap(const GameObject& go) const {
    return go.Component<LightRenderData>().shadowMap;
}

const glm::mat4& GraphicsScene::GetLightCameraProjection(const GameObject& go) const {
    return go.Component<LightRenderData>().camera.invProj;
}
const glm::mat4& GraphicsScene::GetLightCameraView(const GameObject& go) const {
    return go.Component<LightRenderData>().camera.invView;
}

void GraphicsScene::UpdateCameras() {
    for (auto ent : translatedCameras_) {
        auto go{ world_.Get(ent) };

        const auto& camera{ go.Component<CameraComponent>() };
        const auto& transformation{ go.GlobalTransformation() };
        auto& cameraRenderData{ go.Component<CameraRenderData>() };

        UpdateCamera(cameraRenderData, camera, transformation);
    }
    translatedCameras_.clear();

    for (auto ent : updatedCameras_) {
        auto go{ world_.Get(ent) };

        const auto& camera{ go.Component<CameraComponent>() };
        const auto& transformation{ go.GlobalTransformation() };
        auto& cameraRenderData{ go.Component<CameraRenderData>() };

        UpdateCamera(cameraRenderData, camera, transformation);

        const gfxapi::Extent2D cameraExtent{ camera.width, camera.height };
        if (!cameraRenderData.lightCullFrustums || cameraRenderData.extent != cameraExtent) {
            cameraRenderData.extent = cameraExtent;
            cameraRenderData.updateFrustums = true;
        }
    }
    updatedCameras_.clear();
}

void GraphicsScene::UpdateCamera(CameraRenderData& renderData, const CameraComponent& camera, const Transformation& transformation) const {
    switch (camera.projection) {
    case Camera::ProjectionType::Perspective:
        renderData.cCamera = Camera::Perspective(camera.vFovDeg, f32(camera.width), f32(camera.height), camera.zNear, camera.zFar);
        break;
    case Camera::ProjectionType::Ortho:
        // TODO:
        renderData.cCamera = Camera::Ortho(0, 0, f32(camera.width), f32(camera.height), camera.zNear, camera.zFar);
        break;
    }

    renderData.extent = gfxapi::Extent2D{ camera.width, camera.height };
    renderData.camera = CameraShaderData(renderData.cCamera, transformation.Matrix(), transformation.position);

    renderData.cull.frustum = renderData.cCamera.GetFrustum();
}

void GraphicsScene::UpdateCameraFrustums(gfxapi::CommandList& cmd, CameraRenderData& data) const {
    PROFILE_EVENT_NC("Light frustums", COLOR_PROFILE_GRAPHICS);

    UGINE_ASSERT(data.extent.width > 0 && data.extent.height > 0);

    const auto lightGridExtent{ state_.LightGridExtent(data.extent) };
    const u32 numFrustums{ lightGridExtent.width * lightGridExtent.height };
    data.lightCullFrustums = state_.device.CreateBufferUnique(StorageBuffer<shaders::Frustum>("CullLightFrustumsSSBO", numFrustums));

    state_.CalcLightFrustums(cmd, *data.lightCullFrustums, data.extent, data.camera.invProj);
}

WorldHit GraphicsScene::RayCast(const Ray& ray) const {
    PROFILE_EVENT_NC("RayCast", COLOR_PROFILE_GRAPHICS);

    using Clock = std::chrono::high_resolution_clock;
    const auto start{ Clock::now() };

    WorldHit result{};

    auto size{ u32(world_.Registry().view<MeshRenderData>().size()) };
    if (size == 0) {
        return result;
    }

    ParallelRayCast rayCast{
        .ray = ray,
    };

    struct RayCastTask : public Task {
        RayCastTask(u32 num, const GraphicsScene* scene, ParallelRayCast& rayCast)
            : Task{ num } 
            , this_{ scene }
            , rayCast_ { rayCast }
        {}

        void Run(u32 start, u32 end, u32 threadNum) {
            PROFILE_EVENT_NC("MeshRayCast", COLOR_PROFILE_GRAPHICS);

            auto meshes{ this_->world_.Registry().view<MeshRenderData>() };
            for (u32 i{ start }; i < end; ++i) {
                ++rayCast_.perThreadComponentCount[threadNum];

                this_->MeshRayCast(rayCast_, meshes[i]);
            }
        }

        const GraphicsScene* this_{};
        ParallelRayCast& rayCast_;
    };

    auto task{ engine_.FrameAllocator().AlignedNew<RayCastTask>(size, this, rayCast) };
    engine_.GetScheduler().Schedule(rayCast.group, size, task);

    const auto tasks{ engine_.GetScheduler().Wait(rayCast.group) };
    task->~RayCastTask();

    for (const auto& partialResult : rayCast.perThreadResult) {
        if (partialResult.hit && (!result.hit || partialResult.distance < result.distance)) {
            result = partialResult;
        }
    }

    u32 cnt{};
    for (auto c : rayCast.perThreadComponentCount) {
        cnt += c;
    }

    const auto micros{ f32(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count()) };
    UGINE_INFO("RayCast ended in {}ms, with {} tasks and {} tested objects (mesh index = {})", micros / 1000.0f, tasks, cnt, result.meshIndex);

    return result;
}

void GraphicsScene::Update() {
    PROFILE_EVENT_NC("GraphicsScene::Update", COLOR_PROFILE_GRAPHICS);

    frameStats_.drawCalls = 0;
    frameStats_.computeDispatches = 0;

    auto& allocator{ IAllocator::Default() };
    //auto& allocator{ engine_.FrameAllocator() };

    shadowMatrices_.Clear();
    shadowMatrices_.Reserve(world_.Registry().view<LightRenderData>().size());

    // Sync components to render data.
    // TODO: Parallel.
    {
        PROFILE_EVENT_NC("Update render data", COLOR_PROFILE_GRAPHICS);

        UpdateLights();
        UpdateCameras();

        UpdatePendingMeshes();
        UpdateMeshes();

        UpdatePendingAnimations();
        UpdateAnimationControllers();
    }

    schedulerGroup_.Reset();

    UpdateAnimations(schedulerGroup_, engine_.FrameSeconds());

    // Calc visibility lists for each camera.
    for (auto&& [_, renderData] : world_.Registry().view<CameraRenderData>().each()) {
        renderData.visibilityList.Init();
        renderData.cull.group = &schedulerGroup_;

        Cull(renderData.cull, renderData.visibilityList.flags);
    }

    // Calc visibility lists for each shadow caster.
    for (auto&& [entt, renderData, shaderData] : world_.Registry().view<LightRenderData, LightShaderData>().each()) {
        if (!renderData.isShadowCaster) {
            continue;
        }

        auto go{ world_.Get(entt) };

        shaderData.light.shadowIndex = u32(shadowMatrices_.Size());
        shadowMatrices_.EmplaceBack(renderData.camera.viewProj);

        renderData.visibilityList.Init();
        renderData.cull.group = &schedulerGroup_;

        Cull(renderData.cull, renderData.visibilityList.flags);
    }
}

void GraphicsScene::WaitUpdate() {
    WaitFrameTasks();

    // Finalize.
    {
        PROFILE_EVENT_NC("StoreCull", COLOR_PROFILE_GRAPHICS);

        for (auto&& [_, renderData] : world_.Registry().view<CameraRenderData>().each()) {
            StoreCullResults(renderData.cull);
        }

        for (auto&& [_, renderData] : world_.Registry().view<LightRenderData>().each()) {
            StoreCullResults(renderData.cull);
        }
    }
}

void GraphicsScene::StoreCullResults(ParallelCull& cull) const {
    size_t lightSize{};
    size_t meshesSize{};

    for (const auto& partialResult : cull.perThreadResult) {
        lightSize += partialResult.lights.Size();
        meshesSize += partialResult.meshes.Size();
    }

    cull.visibility->lights.Reserve(cull.visibility->lights.Size() + lightSize);
    cull.visibility->meshes.Reserve(cull.visibility->meshes.Size() + meshesSize);

    for (const auto& partialResult : cull.perThreadResult) {
        cull.visibility->drawCalls += partialResult.drawCalls;
        cull.visibility->lights.Append(partialResult.lights);
        cull.visibility->meshes.Append(partialResult.meshes);
    }
}

void GraphicsScene::Cull(ParallelCull& cull, u32 flags) {
    PROFILE_EVENT_NC("Visibility", COLOR_PROFILE_GRAPHICS);

    for (u32 i{}; i < engine_.GetScheduler().NumThreads(); ++i) {
        cull.perThreadResult[i].drawCalls = 0;
        cull.perThreadResult[i].meshes = Vector<GameObjectHandle>{ engine_.FrameAllocator(i) };
        cull.perThreadResult[i].meshes.Reserve(meshesCnt_);
        cull.perThreadResult[i].lights = Vector<GameObjectHandle>{ engine_.FrameAllocator(i) };
        cull.perThreadResult[i].lights.Reserve(world_.Registry().view<LightRenderData>().size());
    }

    //if (flags & VisibilityList::FLAG_LIGHTS) {
    //    CullLights(cull);
    //}

    if (flags & VisibilityList::FLAG_MESHES) {
        CullMeshes(cull);
    }
}

Vector<Draw> GraphicsScene::GetDrawList(const VisibilityList& visibility) const {
    Vector<Draw> drawCalls(engine_.FrameAllocator());
    drawCalls.Reserve(visibility.drawCalls + 1);
    {
        // TODO: Draw calls.
        PROFILE_EVENT_NC("Collect draws", COLOR_PROFILE_GRAPHICS);

        for (auto handle : visibility.meshes) {
            AddMeshDraw(drawCalls, handle);
        }
    }

    for (auto&& [_, renderData] : world_.Registry().view<SkyRenderData>().each()) {
        AddSkyDraw(drawCalls, renderData);
        break;
    }

    return drawCalls;
}

void GraphicsScene::AddMeshDraw(Vector<Draw>& draws, GameObjectHandle handle) const {
    PROFILE_EVENT_NC("AddDraw", COLOR_PROFILE_GRAPHICS);

    auto go{ world_.Get(handle) };
    const auto& renderData{ go.Component<MeshRenderData>() };

    const auto& model{ renderData.modelInstance };
    if (!model.Ready()) {
        return;
    }

    const auto* instanceRenderData{ go.TryGetComponent<InstanceRenderData>() };
    if (instanceRenderData && instanceRenderData->count < 1) {
        return;
    }

    const auto* animatorRenderData{ go.TryGetComponent<AnimatorRenderData>() };

    const auto mModel{ go.GlobalTransformation().Matrix() };
    const auto mNormal{ glm::transpose(glm::inverse(mModel)) };

    if (DebugAabb.Get<bool>()) {
        debugRenderer_.AddCube(renderData.aabb.Min(), renderData.aabb.Max(), glm::vec3{ 0, 1, 0 });
    }

    if (DebugSphere.Get<bool>()) {
        debugRenderer_.AddCircle(renderData.boundingShpere.center, renderData.boundingShpere.radius, glm::vec3{ 1, 0, 0 });
    }

    const u32 flags{ instanceRenderData ? Draw::FLAG_INSTANCED : 0 };
    const u32 variant{ instanceRenderData ? state_.SHADER_INSTANCED_MASK : 0 };

    gfxapi::BufferHandle vertexBuffer{};
    if (animatorRenderData && animatorRenderData->ready) {
        vertexBuffer = animatorRenderData->perFrameSkin[animatorRenderData->updateIndex].vertexBuffer;
    }

    gfxapi::BufferHandle instanceBuffer{};
    if (instanceRenderData) {
        instanceBuffer = instanceRenderData->perFrameInstance[instanceRenderData->updateIndex].instanceBuffer;
    }

    Draw draw{
        .instanceCount = instanceRenderData ? instanceRenderData->count : 1,
        .vertexBuffer = vertexBuffer ? vertexBuffer : model.GetModel()->VertexBuffer(),
        .indexBuffer = model.GetModel()->IndexBuffer(),
        .instanceBuffer = instanceBuffer,
        .indexType = model.GetModel()->IndexType(),
        .stencil = go.GetStencil(),
    };

    for (auto& mesh : model.GetModel()->Meshes()) {
        auto material{ model.GetMaterial(mesh.materialIndex) };
        if (!material) {
            continue;
        }

        material->Prepare(state_, variant | (material->IsTransparent() ? state_.SHADER_OPACITY_MASK : 0));

        draw.flags = flags | (material->IsTransparent() ? Draw::FLAG_TRANSPARENT : 0);
        draw.model = mModel * mesh.transformation;
        draw.normal = mNormal * mesh.transformation;
        draw.indexCount = mesh.indexCount;
        draw.indexOffset = mesh.indexStart;
        draw.vertexOffset = mesh.vertexOffset;

        draw.depthPipeline = material->GetPipeline(variant | state_.SHADER_DEPTH_PASS_MASK);
        draw.depthUniform = material->GetUniform(variant | state_.SHADER_DEPTH_PASS_MASK);

        draw.pipeline = material->GetPipeline(variant);
        draw.uniform = material->GetUniform(variant);

        draw.sortKey = std::hash<ResourceID>{}(material->Id());

        draws.PushBack(draw);
    }
}

void GraphicsScene::AddSkyDraw(Vector<Draw>& draws, const SkyRenderData& renderData) const {
    if (!renderData.material) {
        return;
    }

    constexpr u32 variant{ 0 };

    draws.PushBack(Draw{
        .sortKey = u64(-1), // Render last.
        .model = glm::mat4{ 1.0f },
        .normal = glm::mat4{ 1.0f },
        .indexCount = state_.skyBoxIndexCount,
        .instanceCount = 1,
        .vertexBuffer = *state_.skyBoxVertexBuffer,
        .indexBuffer = *state_.skyBoxIndexBuffer,
        .pipeline = renderData.material->GetPipeline(variant),
        .uniform = renderData.material->GetUniform(variant),
    });
}

void GraphicsScene::CopyLightData(void* dst) const {
    const auto count{ world_.Registry().view<LightShaderData>().size() };
    if (count == 0) {
        return;
    }

    auto& storage{ world_.Registry().storage<LightShaderData>() };
    auto all{ storage.raw() };
    auto pageSize{ entt::component_traits<LightShaderData>::page_size };
    size_t sizeToCopy{ count * sizeof(LightShaderData) };
    size_t offset{};
    int index{};

    auto destination{ reinterpret_cast<char*>(dst) };
    while (sizeToCopy > 0) {
        const auto toCopy{ std::min(sizeof(LightShaderData) * pageSize, sizeToCopy) };
        memcpy(destination, all[index++], toCopy);
        sizeToCopy -= toCopy;
        destination += toCopy;
    }
}

size_t GraphicsScene::LightDataSize() const {
    return std::max<size_t>(1, sizeof(LightShaderData) * world_.Registry().view<LightShaderData>().size());
}

void GraphicsScene::CameraRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) const {
    auto go{ world_.Get(ent) };
    const auto& comp{ go.Component<CameraComponent>() };
    auto& renderData{ go.Component<CameraRenderData>() };

    renderData.updateFrustums = true;
    renderData.visibilityList = VisibilityList{
        .meshes = Vector<GameObjectHandle>{ allocator_ },
        .lights = Vector<GameObjectHandle>{ allocator_ },
    };
    renderData.cull.visibility = &renderData.visibilityList;

    UpdateCamera(renderData, comp, go.GlobalTransformation());
}

void GraphicsScene::LightShaderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) const {
    auto go{ world_.Get(ent) };
    const auto& comp{ go.Component<LightComponent>() };
    auto& shaderData{ go.Component<LightShaderData>() };

    UpdateLightShaderData(shaderData, comp);
    UpdateLightTransformation(shaderData, go.GlobalTransformation());
}

void GraphicsScene::LightRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) const {
    auto go{ world_.Get(ent) };
    const auto& comp{ go.Component<LightComponent>() };
    auto& renderData{ go.Component<LightRenderData>() };

    renderData.cull.visibility = &renderData.visibilityList;

    UpdateLightAabb(renderData, comp, go.GlobalTransformation());
    UpdateLightRenderData(renderData, comp, go.GlobalTransformation());
}

void GraphicsScene::UpdateLightAabb(LightRenderData& renderData, const LightComponent& light, const Transformation& transformation) const {
    switch (light.type) {
    case LightComponent::Type::Directional:
        renderData.aabb = AABB{ glm::vec3{ std::numeric_limits<f32>::min() }, glm::vec3{ std::numeric_limits<f32>::max() } };
        break;
    case LightComponent::Type::Spot:
        // TODO:
        renderData.aabb = AABB{ transformation.position - light.range, transformation.position + light.range };
        break;
    case LightComponent::Type::Point:
        //
        renderData.aabb = AABB{ transformation.position - light.range, transformation.position + light.range };
        break;
    };
}

void GraphicsScene::MeshModelReady(GameObject& go) const {
    auto& renderData{ go.Component<MeshRenderData>() };
    renderData.modelReady = true;
    meshesCnt_ += u32(renderData.modelInstance.GetModel()->Meshes().Size());

    UpdateMeshAabb(go);

    // Animations.
    auto animatorRenderData{ go.TryGetComponent<AnimatorRenderData>() };
    if (animatorRenderData) {
        InitAnimatorRenderData(renderData.modelInstance, *animatorRenderData);
    }
}

void GraphicsScene::UpdatePendingMeshes() {
    for (auto&& [ent, renderData] : world_.Registry().view<MeshRenderData, PendingModelFlag>().each()) {
        if (renderData.modelInstance.Ready()) {
            auto go{ world_.Get(ent) };

            MeshModelReady(go);
            go.RemoveComponent<PendingModelFlag>();
        }
    }
}

void GraphicsScene::UpdateMeshes() {
    for (auto ent : updatedMeshes_) {
        auto go{ world_.Get(ent) };

        UGINE_DEBUG("Mesh updated: {}", go.Name());

        const auto& mesh{ go.Component<MeshComponent>() };
        auto& renderData{ go.Component<MeshRenderData>() };
        auto animatorRenderData{ go.TryGetComponent<AnimatorRenderData>() };

        // Instance data.
        if (mesh.instanced) {
            // TODO: Update AABB.
            auto& instanceRenderData{ go.GetOrCreateComponent<InstanceRenderData>() };

            // TODO: Copy only on change.
            UpdateMeshInstancedData(instanceRenderData, mesh);
        } else if (go.Has<InstanceRenderData>()) {
            go.RemoveComponent<InstanceRenderData>();
        }

        // Model changed.
        if (renderData.modelInstance != mesh.modelInstance) {
            if (renderData.modelInstance.GetModel()) {
                UGINE_ASSERT(meshesCnt_ >= renderData.modelInstance.GetModel()->Meshes().Size());
                meshesCnt_ -= u32(renderData.modelInstance.GetModel()->Meshes().Size());
            }

            renderData.modelInstance = mesh.modelInstance;
            if (renderData.modelInstance.Ready()) {
                go.RemoveComponent<PendingModelFlag>();

                MeshModelReady(go);
            } else if (!go.Has<PendingModelFlag>()) {
                go.CreateComponent<PendingModelFlag>();
            }
        }
    }

    updatedMeshes_.clear();

    for (auto ent : translatedMeshes_) {
        auto go{ world_.Get(ent) };
        const auto& renderData{ go.Component<MeshRenderData>() };

        if (renderData.modelReady) {
            UpdateMeshAabb(go);
        }
    }

    translatedMeshes_.clear();
}

void GraphicsScene::MeshCreated(GameObjectRegistry& reg, GameObjectHandle ent) {
    auto go{ world_.Get(ent) };
    const auto& mesh{ go.Component<MeshComponent>() };
    auto& renderData{ go.Component<MeshRenderData>() };

    UGINE_DEBUG("Mesh renderdata created: {}", go.Name());

    renderData.modelReady = mesh.modelInstance.Ready();
    renderData.modelInstance = mesh.modelInstance;

    if (mesh.instanced) {
        UpdateMeshInstancedData(go.CreateComponent<InstanceRenderData>(), mesh);
    }

    if (renderData.modelReady) {
        MeshModelReady(go);
    } else {
        go.CreateComponent<PendingModelFlag>();
    }
}

void GraphicsScene::MeshDestroyed(GameObjectRegistry& reg, GameObjectHandle ent) {
    auto go{ world_.Get(ent) };
    auto& renderData{ go.Component<MeshRenderData>() };

    if (renderData.modelReady) {
        UGINE_ASSERT(meshesCnt_ >= renderData.modelInstance.GetModel()->Meshes().Size());
        meshesCnt_ -= u32(renderData.modelInstance.GetModel()->Meshes().Size());
    }
}

void GraphicsScene::InstancedRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) {
    UGINE_DEBUG("Instance render data created.");

    auto go{ world_.Get(ent) };
    auto& renderData{ go.Component<InstanceRenderData>() };

    renderData.count = 0;
    renderData.updateIndex = 0;
    renderData.perFrameInstance.Resize(state_.framesInFlight);
}

void GraphicsScene::InstancedRenderDataDestroyed(GameObjectRegistry& reg, GameObjectHandle ent) {
    UGINE_DEBUG("Instance render data destroyed.");

    auto go{ world_.Get(ent) };
    auto& renderData{ go.Component<InstanceRenderData>() };

    for (auto& perFrame : renderData.perFrameInstance) {
        if (perFrame.instanceBuffer) {
            state_.device.DestroyBuffer(perFrame.instanceBuffer);
        }
    }
}

void GraphicsScene::UpdatePendingAnimations() {
    for (auto&& [ent, renderData] : world_.Registry().view<AnimatorRenderData, PendingAnimationFlag>().each()) {
        UGINE_ASSERT(renderData.animation);

        if (renderData.animation->Ready()) {
            renderData.ready = true;

            world_.Get(ent).RemoveComponent<PendingAnimationFlag>();
        }
    }
}

void GraphicsScene::UpdateAnimationControllers() {
    for (auto ent : updatedAnimationControllers_) {
        auto go{ world_.Get(ent) };

        const auto& ac{ go.Component<AnimationControllerComponent>() };
        auto& renderData{ go.Component<AnimatorRenderData>() };

        renderData.animation = ac.animation;
        renderData.ready = renderData.animation && renderData.animation->Ready();

        if (renderData.animation && !renderData.animation->Ready()) {
            go.CreateComponent<PendingAnimationFlag>();
        }
    }

    updatedAnimationControllers_.clear();
}

void GraphicsScene::InitAnimatorRenderData(const ModelInstance& model, AnimatorRenderData& renderData) const {
    if (!model.HasBones()) {
        return;
    }

    auto modelPtr{ model.GetModel().Get() };
    UGINE_ASSERT(modelPtr);

    if (renderData.boneMatrices.Size() < modelPtr->Bones().Size()) {
        renderData.boneMatrices.Resize(modelPtr->Bones().Size());

        for (auto& matrix : renderData.boneMatrices) {
            matrix = glm::mat4{ 1.0f };
        }
    }

    if (!renderData.perFrameSkin.Empty() && renderData.vertexBufferSize >= modelPtr->VertexBufferSize()) {
        return;
    }

    for (auto& frame : renderData.perFrameSkin) {
        if (frame.vertexBuffer) {
            state_.device.DestroyBuffer(frame.vertexBuffer);
        }
        if (frame.matrixBuffer) {
            state_.device.DestroyBuffer(frame.matrixBuffer);
        }
    }

    renderData.perFrameSkin.Resize(state_.framesInFlight);
    renderData.vertexBufferSize = modelPtr->VertexBufferSize();

    const BufferDesc vertexBufDesc{
        .name = "SkinnedMeshComponentVertices",
        .flags = BufferFlags::Vertex | BufferFlags::Storage,
        .size = renderData.vertexBufferSize,
    };

    const BufferDesc matrixBufDesc{
        .name = "SkinnedMeshComponentMatrices",
        .flags = BufferFlags::Storage,
        .size = u32(sizeof(glm::mat4) * modelPtr->Bones().Size()),
        .cpuAccess = CpuAccessFlags::Write,
    };

    for (u32 i{}; i < state_.framesInFlight; ++i) {
        renderData.perFrameSkin[i].vertexBuffer = state_.device.CreateBuffer(vertexBufDesc);
        renderData.perFrameSkin[i].matrixBuffer = state_.device.CreateBuffer(matrixBufDesc, renderData.boneMatrices.Data(), renderData.boneMatrices.DataSize());
    }
}

void GraphicsScene::UpdateMeshInstancedData(InstanceRenderData& renderData, const MeshComponent& mesh) {
    PROFILE_EVENT_NC("Update instance", COLOR_PROFILE_GRAPHICS);
    UGINE_DEBUG("Update mesh instance data");

    renderData.count = u32(mesh.instanceTransformations.size());
    if (renderData.count == 0) {
        return;
    }

    renderData.updateIndex = (renderData.updateIndex + 1) % renderData.perFrameInstance.Size();

    auto& frame{ renderData.perFrameInstance[renderData.updateIndex] };

    const auto instanceBufferSize{ u32(sizeof(glm::mat4) * mesh.instanceTransformations.size()) };
    if (instanceBufferSize > frame.instanceBufferSize) {
        if (frame.instanceBuffer) {
            state_.device.DestroyBuffer(frame.instanceBuffer);
        }

        const BufferDesc desc{
            .name = "MeshComponentInstanceTransformations",
            .flags = BufferFlags::Vertex,
            .size = instanceBufferSize,
            .cpuAccess = CpuAccessFlags::Write,
        };

        frame.instanceBufferSize = instanceBufferSize;
        frame.instanceBuffer = state_.device.CreateBuffer(desc, mesh.instanceTransformations.data(), instanceBufferSize);
    } else {
        memcpy(state_.device.GetBufferMapped(frame.instanceBuffer), mesh.instanceTransformations.data(), instanceBufferSize);
    }
}

void GraphicsScene::PrepareRenderData(gfxapi::CommandList& cmd) {
    PROFILE_EVENT_NC("PrepareRenderData", COLOR_PROFILE_GRAPHICS);

    for (auto&& [ent, sky, renderData] : world_.Registry().view<SkyComponent, SkyRenderData>().each()) {
        if (sky.material != renderData.material && sky.material->Ready()) {
            // TODO: Update prefiltered cube etc., for IBL.
            renderData.material = sky.material;
            renderData.material->Prepare(state_, 0);
        }

        break;
    }

    // Global shader data.
    global_.time = f32(engine_.FrameSeconds()); // TODO:
    global_.lightCount = u32(world_.Registry().view<LightShaderData>().size());

    gpuGlobal_ = cmd.AllocateGPU(sizeof(shaders::Global));
    *gpuGlobal_.As<shaders::Global>() = global_;

    // Light data.

    // TODO: On change only.
    gpuLightsSB_ = cmd.AllocateGPU(LightDataSize());
    CopyLightData(gpuLightsSB_.mapped);

    if (!shadowMatrices_.Empty()) {
        const auto shadowDataSize{ shadowMatrices_.DataSize() };
        gpuShadowsSB_ = cmd.AllocateGPU(shadowDataSize);
        memcpy(gpuShadowsSB_.mapped, shadowMatrices_.Data(), shadowDataSize);
    } else {
        // TODO:
        gpuShadowsSB_ = cmd.AllocateGPU(1);
    }

    // Animations.
    UploadAnimationRenderData(cmd);

    // Cameras.
    for (auto&& [_, renderData] : world_.Registry().view<CameraRenderData>().each()) {
        if (renderData.updateFrustums) {
            UpdateCameraFrustums(cmd, renderData);
            renderData.updateFrustums = false;
        }

        if (renderData.customRtv) {
            if (!renderData.depthBuffer) {
                renderData.depthBuffer = state_.GetRtv(renderData.rtvExtent, state_.DEPTH_STENCIL_FORMAT,
                    TextureUsageFlags::DepthStencil | TextureUsageFlags::Sampled, "CameraRenderDataDepthStencil");
            }
        } else if (renderData.rtvExtent != renderData.extent) {
            renderData.rtvExtent = renderData.extent;
            renderData.renderTarget
                = state_.GetRtv(renderData.rtvExtent, state_.LDR_FORMAT, TextureUsageFlags::RenderTarget | TextureUsageFlags::Sampled, "CameraRenderDataRTV");
            renderData.depthBuffer = state_.GetRtv(renderData.rtvExtent, state_.DEPTH_STENCIL_FORMAT,
                TextureUsageFlags::DepthStencil | TextureUsageFlags::Sampled, "CameraRenderDataDepthStencil");
        }
    }

    // Shadows.
    for (auto&& [ent, renderData] : world_.Registry().view<LightRenderData>().each()) {
        if (!renderData.isShadowCaster) {
            continue;
        }

        auto go{ world_.Get(ent) };
        auto& shaderData{ go.Component<LightShaderData>() };

        auto shadowMap{ state_.GetRtv(
            state_.shadowMapResolution, state_.DEPTH_STENCIL_FORMAT, TextureUsageFlags::DepthStencil | TextureUsageFlags::Sampled, "LightCameraDepthStencil") };
        renderData.shadowMap = shadowMap;
        shaderData.light.shadowMapIndex = state_.device.GetTextureBindlessIndex(shadowMap);
    }
}

void GraphicsScene::AnimatorRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) {
    auto go{ world_.Get(ent) };
    auto& renderData{ go.Component<AnimatorRenderData>() };

    UGINE_DEBUG("AnimatorRenderData created: {}", go.Name());

    renderData.perFrameSkin.Resize(state_.framesInFlight);
    renderData.syncAnimation = false;

    if (auto meshRenderData{ go.TryGetComponent<MeshRenderData>() }; meshRenderData) {
        if (meshRenderData->modelInstance.Ready() && meshRenderData->modelInstance.HasBones()) {
            InitAnimatorRenderData(meshRenderData->modelInstance, renderData);
        }
    }
}

void GraphicsScene::AnimatorRenderDataDestroyed(GameObjectRegistry& reg, GameObjectHandle ent) {
    auto& renderData{ reg.get<AnimatorRenderData>(ent) };

    for (auto& frame : renderData.perFrameSkin) {
        if (frame.vertexBuffer) {
            state_.device.DestroyBuffer(frame.vertexBuffer);
        }
        if (frame.matrixBuffer) {
            state_.device.DestroyBuffer(frame.matrixBuffer);
        }
    }
}

void GraphicsScene::UpdateAnimations(Scheduler::Group& group, f64 frameSeconds) {
    auto size{ u32(world_.Registry().view<AnimatorRenderData>().size()) };
    if (size == 0) {
        return;
    }

    PROFILE_EVENT_NC("UpdateAnimations", COLOR_PROFILE_GRAPHICS);

    struct AnimationsTask : public Task {
        AnimationsTask(u32 num, GraphicsScene* scene, f64 frameSeconds)
            : Task{ num } 
            , this_{scene}
            , frameSeconds_{frameSeconds}
        {}

        void Run(u32 start, u32 end, u32 threadNum) override {
            auto animations{ this_->world_.Registry().view<AnimatorRenderData>() };

            for (u32 i{ start }; i < end; ++i) {
                auto go{ animations[i] };
                for (auto&& [ent, renderData, animatorRenderData, animationController] :
                    this_->world_.Registry().view<MeshRenderData, AnimatorRenderData, AnimationControllerComponent>().each()) {

                    if (!renderData.modelReady || !animatorRenderData.ready) {
                        continue;
                    }

                    PROFILE_EVENT_NC("Animation", COLOR_PROFILE_GRAPHICS);

                    if (animatorRenderData.boneMatrices.Empty()) {
                        continue;
                    }

                    if (frameSeconds_ - animatorRenderData.lastUpdateTimeS < animationController.resolutionS) {
                        continue;
                    }

                    animatorRenderData.lastUpdateTimeS = frameSeconds_;
                    if (animationController.isRunning) {
                        // TODO: f32
                        animationController.animationTime
                            = animationController.animation->lengthSeconds == 0 ? 0 : fmod(f32(frameSeconds_), animationController.animation->lengthSeconds);
                    }

                    UpdateAnimation(this_->engine_.FrameAllocator(), *renderData.modelInstance.GetModel().Get(), *animationController.animation.Get(),
                        animationController.animationTime, animatorRenderData.boneMatrices.ToSpan());

                    animatorRenderData.syncAnimation = true;
                }
            }
        }

        GraphicsScene* this_{};
        f64 frameSeconds_{};
    };

    auto task{ NewFrameTask<AnimationsTask>(size, this, frameSeconds) };
    engine_.GetScheduler().Schedule(group, size, task);
}

void GraphicsScene::UploadAnimationRenderData(gfxapi::CommandList& cmd) {
    // TODO:

    PROFILE_EVENT_NC("AnimationsRenderData", COLOR_PROFILE_GRAPHICS);

    UGINE_GPU_EVENT(cmd, label, "RenderData UpdateAnimations");

    bool psoBound{};
    for (auto&& [ent, _, renderData] : world_.Registry().view<MeshRenderData, AnimatorRenderData>().each()) {
        if (!renderData.syncAnimation) {
            continue;
        }

        renderData.syncAnimation = false;

        renderData.updateIndex = (renderData.updateIndex + 1) % renderData.perFrameSkin.Size();
        auto& frameData{ renderData.perFrameSkin[renderData.updateIndex] };
        UGINE_ASSERT(!renderData.boneMatrices.Empty());

        memcpy(state_.device.GetBufferMapped(frameData.matrixBuffer), renderData.boneMatrices.Data(), renderData.boneMatrices.DataSize());

        if (!psoBound) {
            cmd.BindPipeline(*state_.animationCSO);
            psoBound = true;
        }

        auto go{ world_.Get(ent) };
        const auto& meshRenderData{ go.Component<MeshRenderData>() };
        auto modelPtr{ meshRenderData.modelInstance.GetModel().Get() };

        struct alignas(16) Push {
            u32 count;
        } push{ modelPtr->VertexCount() };

        cmd.BindStorage(0, 0, modelPtr->VertexBuffer());
        cmd.BindStorage(0, 1, modelPtr->SkinnedBuffer());
        cmd.BindStorage(0, 2, frameData.matrixBuffer);
        cmd.BindStorage(0, 3, frameData.vertexBuffer);
        cmd.PushConstants(ShaderStage::ComputeShader, 0, push);

        cmd.Dispatch(1 + static_cast<u32>((modelPtr->VertexCount()) / 256), 1, 1);
        ++frameStats_.computeDispatches;

        cmd.Barrier(BufferBarrier{
            .buffer = frameData.vertexBuffer,
            .offset = 0,
            .size = modelPtr->VertexBufferSize(),
            .srcAccess = AccessFlags::ShaderWrite,
            .srcStage = PipelineStageFlags::ComputeShader,
            .dstAccess = AccessFlags::VertexAttributeRead,
            .dstStage = PipelineStageFlags::VertexInput,
        });
    }
}

void ugine::GraphicsScene::UpdateLights() {
    PROFILE_EVENT_NC("UpdateLights", COLOR_PROFILE_GRAPHICS);

    for (auto ent : updatedLightTransformations_) {
        auto go{ world_.Get(ent) };
        const auto& light{ go.Component<LightComponent>() };
        auto& shaderData{ go.Component<LightShaderData>() };
        auto& renderData{ go.Component<LightRenderData>() };

        UpdateLightTransformation(shaderData, go.GlobalTransformation());
        UpdateLightAabb(renderData, light, go.GlobalTransformation());
        UpdateLightShaderData(shaderData, light);
        UpdateLightRenderData(renderData, light, go.GlobalTransformation());
    }

    updatedLightTransformations_.clear();

    for (auto ent : updatedLights_) {
        auto go{ world_.Get(ent) };
        UGINE_ASSERT(go.Has<LightComponent>());
        UGINE_ASSERT(go.Has<LightShaderData>());

        const auto& light{ go.Component<LightComponent>() };
        auto& shaderData{ go.Component<LightShaderData>() };
        auto& renderData{ go.Component<LightRenderData>() };

        UpdateLightShaderData(shaderData, light);
        UpdateLightRenderData(renderData, light, go.GlobalTransformation());
    }

    updatedLights_.clear();
}

void GraphicsScene::UpdateLightShaderData(LightShaderData& shaderData, const LightComponent& light) const {
    auto& l{ shaderData.light };

    l.color = glm::vec3{ light.color };
    l.intensity = light.intensity;
    l.spotAngleRad = glm::radians(light.spotAngleDeg);
    l.range = light.range;

    // TODO:
    l.shadowMapIndex = -1;
    l.shadowIndex = -1;

    switch (light.type) {
    case LightComponent::Type::Directional: l.typeEnabled = LIGHT_DIRECTION; break;
    case LightComponent::Type::Point: l.typeEnabled = LIGHT_POINT; break;
    case LightComponent::Type::Spot: l.typeEnabled = LIGHT_SPOT; break;
    }
    l.typeEnabled = (l.typeEnabled << 1) | 0x1;
}

void GraphicsScene::UpdateLightRenderData(LightRenderData& renderData, const LightComponent& light, const Transformation& transformation) const {
    renderData.isShadowCaster = light.generatesShadows;

    auto lightCamera{ LightCamera(light, state_.shadowMapResolution) };

    glm::mat4 invViewMatrix{};
    if (light.type == LightComponent::Type::Directional) {
        auto invDir{ -transformation.rotation * math::FORWARD };
        invViewMatrix = glm::inverse(glm::lookAtRH(invDir, glm::vec3{ 0.0f }, math::UP));
    } else {
        invViewMatrix = glm::inverse(transformation.Matrix());
    }

    renderData.camera = CameraShaderData(lightCamera, invViewMatrix, transformation.position);
    renderData.cull.frustum = lightCamera.GetFrustum();
}

void GraphicsScene::UpdateLightTransformation(LightShaderData& l, const Transformation& transformation) const {
    l.light.positionWS = transformation.position;
    l.light.direction = glm::normalize(transformation.rotation * math::FORWARD);
}

//void GraphicsScene::CullLights(ParallelCull& cull) const {
//    const auto size{ u32(world_.Registry().view<LightRenderData>().size()) };
//
//    engine_.GetScheduler().Schedule(*cull.group, size, [this, &cull](u32 start, u32 end, u32 numThread) {
//        PROFILE_EVENT_NC("CullLights", COLOR_PROFILE_GRAPHICS);
//
//        auto lights{ world_.Registry().view<LightRenderData>() };
//        for (u32 i{ start }; i < end; ++i) {
//            auto go{ world_.Get(lights[i]) };
//
//            const auto& light{ go.Component<LightComponent>() };
//            const auto& renderData{ go.Component<LightRenderData>() };
//
//            if (go.IsEnabled() && (light.type == LightComponent::Type::Directional || AabbInFrustum(cull.frustum, renderData.aabb))) {
//                cull.perThreadResult[numThread].lights.PushBack(go);
//
//                if (light.generatesShadows) {
//                    cull.perThreadResult[numThread].shadowCasters.PushBack(go);
//                }
//            }
//        }
//    });
//}

void GraphicsScene::CullMeshes(ParallelCull& cull) {
    struct CullMeshTask : public Task {
        CullMeshTask(u32 size, const GraphicsScene* scene, ParallelCull& cull)
            : Task{ size }
            , this_{ scene }
            , cull_{ cull } {}

        void Run(u32 start, u32 end, u32 numThread) override {
            PROFILE_EVENT_NC("CullMeshes", COLOR_PROFILE_GRAPHICS);

            auto& result{ cull_.perThreadResult[numThread] };

            auto meshes{ this_->world_.Registry().view<MeshRenderData>() };
            for (u32 i{ start }; i < end; ++i) {
                auto mesh{ this_->world_.Get(meshes[i]) };
                auto& renderData{ mesh.Component<MeshRenderData>() };

                if (renderData.modelReady && mesh.IsEnabled() /*&& AabbInFrustum(cull.frustum, renderData.aabb)*/) {
                    result.meshes.PushBack(meshes[i]);
                    result.drawCalls += u32(renderData.modelInstance.GetModel()->Meshes().Size());
                }
            }
        }

        const GraphicsScene* this_{};
        ParallelCull& cull_;
    };

    const auto size{ u32(world_.Registry().view<MeshRenderData>().size()) };
    if (size) {
        auto task{ NewFrameTask<CullMeshTask>(size, this, cull) };

        engine_.GetScheduler().Schedule(*cull.group, size, task);
    }
}

void GraphicsScene::SkyCreated(GameObjectRegistry& reg, GameObjectHandle ent) {
    // TODO:
}

void GraphicsScene::UpdateMeshAabb(GameObject& go) const {
    UGINE_ASSERT(go.Has<MeshRenderData>());
    auto& renderData{ go.Component<MeshRenderData>() };

    UGINE_ASSERT(renderData.modelReady);

    renderData.modelMatrix = go.GlobalTransformation().Matrix();
    renderData.invModelMatrix = glm::inverse(renderData.modelMatrix);

    auto model{ renderData.modelInstance.GetModel().Get() };

    // TODO: Instanced AABB.
    //if (renderData.instance.instanced) {
    //    const auto modelAabb{ model->BoundingBox() };

    //    AABB aabb{};
    //    for (u32 i{}; i < renderData.instance.count; ++i) {
    //        const auto instanceAabb{ modelAabb.Transform(ToMatrix(meshComp.instanceTransformations[i])) };
    //        aabb = aabb.Merge(instanceAabb);
    //    }

    //    renderData.aabb = aabb.Transform(renderData.modelMatrix);
    //} else {
    //    renderData.aabb = model->BoundingBox().Transform(renderData.modelMatrix);
    //}

    renderData.aabb = model->BoundingBox().Transform(renderData.modelMatrix);

    renderData.boundingShpere.center = renderData.aabb.CenterPoint();
    renderData.boundingShpere.radius = renderData.aabb.Diagonal() / 2.0f;
    renderData.aabbReady = true;
}

void GraphicsScene::MeshRayCast(ParallelRayCast& rayCast, GameObjectHandle handle) const {
    PROFILE_EVENT_NC("MeshRayCast", COLOR_PROFILE_GRAPHICS);

    auto go{ world_.Get(handle) };
    const auto& renderData{ go.Component<MeshRenderData>() };
    const auto& meshComponent{ go.Component<MeshComponent>() };

    auto hit{ RaySphereIntersect(rayCast.ray, renderData.boundingShpere) };
    if (!hit.hit) {
        return;
    }

    hit = RayAabbIntersect(rayCast.ray, renderData.aabb);
    if (!hit.hit) {
        return;
    }

    const auto size{ u32(meshComponent.modelInstance.GetModel()->Meshes().Size()) };
    engine_.GetScheduler().ScheduleStatic(rayCast.group, size, [this, &rayCast, handle](u32 start, u32 end, u32 numThread) {
        for (u32 meshIndex{ start }; meshIndex < end; ++meshIndex) {
            auto go{ world_.Get(handle) };
            const auto& meshComponent{ go.Component<MeshComponent>() };
            const auto& mesh{ meshComponent.modelInstance.GetModel()->Meshes()[meshIndex] };
            const auto& renderData{ go.Component<MeshRenderData>() };

            // Use ray in mesh local space so it's not neccessary to transform each vertex of mesh.
            const auto meshToWorld{ renderData.modelMatrix * mesh.transformation };
            const auto meshLocalRay{ rayCast.ray.Transform(glm::inverse(meshToWorld)) };

            engine_.GetScheduler().ScheduleStatic(
                rayCast.group, mesh.indexCount / 3, [this, &rayCast, handle, meshIndex, meshLocalRay, meshToWorld](u32 start, u32 end, u32 numThread) {
                    PROFILE_EVENT_NC("MeshVertexRayCast", COLOR_PROFILE_GRAPHICS);

                    auto go{ world_.Get(handle) };
                    const auto& meshComponent{ go.Component<MeshComponent>() };
                    const auto& mesh{ meshComponent.modelInstance.GetModel()->Meshes()[meshIndex] };

                    for (u32 index{ start * 3 }, last{ end * 3 }; index < last; index += 3) {
                        UGINE_ASSERT(index <= mesh.indices.Size() - 3);

                        const auto& p0{ mesh.vertices[mesh.indices[index + 0]] };
                        const auto& p1{ mesh.vertices[mesh.indices[index + 1]] };
                        const auto& p2{ mesh.vertices[mesh.indices[index + 2]] };

                        auto& result{ rayCast.perThreadResult[numThread] };
                        const auto triangleHit{ RayTriangleIntersect(meshLocalRay, p0, p1, p2) };
                        if (!triangleHit.hit) {
                            continue;
                        }

                        // Transform mesh local hit point back to world space.
                        const auto meshLocalHitPoint{ meshLocalRay.origin + meshLocalRay.dir * triangleHit.distance };
                        const glm::vec3 worldHitPoint{ meshToWorld * glm::vec4{ meshLocalHitPoint, 1.0f } };
                        const float worldDistance{ glm::length(rayCast.ray.origin - worldHitPoint) };

                        if (!result.hit || worldDistance < result.distance) {
                            result.hit = true;
                            result.distance = worldDistance;
                            result.object = handle;
                            result.point = worldHitPoint;
                            result.uv = triangleHit.uv;
                            result.triangle[0] = meshToWorld * glm::vec4{ p0, 1.0f };
                            result.triangle[1] = meshToWorld * glm::vec4{ p1, 1.0f };
                            result.triangle[2] = meshToWorld * glm::vec4{ p2, 1.0f };
                            result.meshIndex = meshIndex;
                        }
                    }
                });
        };
    });
}

shaders::Camera GraphicsScene::CameraShaderData(const Camera& camera, const glm::mat4& inverseViewMatrix, const glm::vec3& position) {
    shaders::Camera cameraCB{};

    cameraCB.proj = camera.ProjectionMatrix();
    cameraCB.invView = inverseViewMatrix;
    cameraCB.invProj = glm::inverse(cameraCB.proj);
    cameraCB.view = glm::inverse(cameraCB.invView);
    cameraCB.viewProj = cameraCB.proj * cameraCB.view;
    cameraCB.position = position;
    cameraCB.zNear = camera.Near();
    cameraCB.zFar = camera.Far();
    cameraCB.resolution = glm::vec2{ camera.Width(), camera.Height() };
    cameraCB.aoTexture = -1; // TODO:

    return cameraCB;
}

void GraphicsScene::Render(gfxapi::CommandList& cmd) {
    WaitUpdate();

    UpdateGpuStats(cmd);

    PrepareRenderData(cmd);

    PROFILE_EVENT_NC("Render", COLOR_PROFILE_GRAPHICS);

    {
        GpuScopeQuery query{ cmd, QueryPool(), gpuQueries_.shadow };

        for (auto&& [shadowCaster, renderData] : world_.Registry().view<LightRenderData>().each()) {
            if (renderData.isShadowCaster) {
                RenderShadow(cmd, world_.Get(shadowCaster));
            }
        }
    }

    for (auto camera : world_.Registry().view<CameraComponent>()) {
        RenderCamera(cmd, world_.Get(camera));
    }

    // Clear on frame end so other systems can use it.
    debugRenderer_.Clear();
}

void GraphicsScene::RenderCamera(gfxapi::CommandList& cmd, const GameObject& cameraGO) {
    PROFILE_EVENT_NC("RenderCamera", COLOR_PROFILE_GRAPHICS);

    const auto& camera{ cameraGO.Component<CameraComponent>() };
    const auto& renderData{ cameraGO.Component<CameraRenderData>() };

    // Per camera data.
    auto gpuCameraCB{ cmd.AllocateGPU(sizeof(shaders::Camera)) };
    *gpuCameraCB.As<shaders::Camera>() = renderData.camera;

    auto draws{ GetDrawList(renderData.visibilityList) };

    if (!DisableDrawSort.GetBool()) {
        std::sort(draws.begin(), draws.end(), [](const auto& a, const auto& b) { return a.sortKey < b.sortKey; });
    }

    // Render camera.
    RenderContext context{ 
        .state = &state_,
        .extent = renderData.rtvExtent,
        .clearColor = camera.clearColor,
        .clearDepth = camera.clearDepth,
        .clearStencil = camera.clearStencil,
        .renderTargetHDR = state_.GetRtv(renderData.rtvExtent, state_.HDR_FORMAT),
        .depthBuffer = renderData.depthBuffer,
        .postprocessPingPong = {
            renderData.renderTarget, // TODO:
            renderData.renderTarget, // TODO:
        },
        .globalCB = global_,
        .gpuGlobalCB = gpuGlobal_,
        .gpuLightListSB = gpuLightsSB_,
        .gpuShadowsSB = gpuShadowsSB_,
        .cameraCB = renderData.camera,
        .gpuCameraCB = gpuCameraCB,
        .gpuLightCullFrustums = *renderData.lightCullFrustums,
        .draws = draws.ToSpan(),
    };

    {
        GpuScopeQuery query{ cmd, QueryPool(), gpuQueries_.depth };

        state_.depthPrePass->RenderDepth(cmd, context);
    }

    {
        GpuScopeQuery query{ cmd, QueryPool(), gpuQueries_.lightCull };

        state_.lightCullingPass->CullLights(cmd, context);
    }

    if (DebugLightCull.Get<int>() > 0) {
        state_.lightCullingPass->DebugLights(cmd, context, DebugLightCull.Get<int>() == 2);
        return;
    }

    {
        GpuScopeQuery query{ cmd, QueryPool(), gpuQueries_.ao };

        if (!DisableSSAO.Get<bool>()) {
            state_.aoPass->Render(cmd, context);

            // TODO:
            cameraGO.Component<CameraRenderData>().camera.aoTexture = state_.device.GetTextureBindlessIndex(context.aoTexture);
        }
    }

    cmd.Barrier(ImageBarrier{
        .texture = context.depthBuffer,
        .srcLayout = TextureLayout::ReadOnly,
        .srcAccess = AccessFlags::ShaderRead,
        .srcStage = PipelineStageFlags::ComputeShader | PipelineStageFlags::FragmentShader,
        .dstLayout = TextureLayout::Attachment,
        .dstAccess = AccessFlags::DepthStencilAttachmentRead,
        .dstStage = PipelineStageFlags::EarlyFragmentTests | PipelineStageFlags::LateFragmentTests,
    });

    // TODO: Dynamic rendering and render pass dependency.
    {
        GpuScopeQuery query{ cmd, QueryPool(), gpuQueries_.geometry };

        state_.forwardPass->Render(cmd, context);
    }

    cmd.Barrier(ImageBarrier{
        .texture = context.renderTargetHDR,
        .srcLayout = TextureLayout::ReadOnly,
        .srcAccess = AccessFlags::ColorAttachmentWrite,
        .srcStage = PipelineStageFlags::ColorAttachmentOutput,
        .dstLayout = TextureLayout::ReadOnly,
        .dstAccess = AccessFlags::ShaderRead,
        .dstStage = PipelineStageFlags::FragmentShader,
    });

    {
        GpuScopeQuery query{ cmd, QueryPool(), gpuQueries_.postProcess };

        // Postprocess.
        state_.tonemappingPass->Render(cmd, context);
    }

    // TODO: Debug renderer.

    frameStats_.drawCalls += context.drawCallsCounter;
    frameStats_.computeDispatches += context.computeDispatches;
}

void GraphicsScene::RenderShadow(gfxapi::CommandList& cmd, const GameObject& go) {
    PROFILE_EVENT_NC("RenderShadow", COLOR_PROFILE_GRAPHICS);

    const auto& renderData{ go.Component<LightRenderData>() };
    auto draws{ GetDrawList(renderData.visibilityList) };
    frameStats_.drawCalls += u32(draws.Size());

    //state_.renderThread.PushRenderWork([this, draws = std::move(draws), &renderData](gfxapi::CommandList& cmd) mutable {

    if (!DisableDrawSort.GetBool()) {
        std::sort(draws.begin(), draws.end(), [](const auto& a, const auto& b) { return a.sortKey < b.sortKey; });
    }

    auto gpuCameraCB{ cmd.AllocateGPU(sizeof(shaders::Camera)) };
    *gpuCameraCB.As<shaders::Camera>() = renderData.camera;

    RenderContext context{
        .state = &state_,
        .extent = state_.shadowMapResolution,
        .clearDepth = 1.0f,
        .depthBuffer = renderData.shadowMap,
        .globalCB = global_,
        .gpuGlobalCB = gpuGlobal_,
        .cameraCB = renderData.camera,
        .gpuCameraCB = gpuCameraCB,
        .draws = draws.ToSpan(),
    };

    state_.shadowPass->RenderShadows(cmd, context);

    frameStats_.drawCalls += context.drawCallsCounter;
    frameStats_.computeDispatches += context.computeDispatches;
    //});
}

void GraphicsScene::UpdateGpuStats(gfxapi::CommandList& cmd) {
    auto pool{ QueryPool() };
    auto results{ state_.device.FetchQueryPoolResults(pool) };

    auto resolve
        = [this](auto& results, const GpuQuery& query) { return float(state_.device.GetMicroseconds(results[query.end] - results[query.begin]) / 1000.0f); };
    gpuFrameStats_.shadowsMS = resolve(results, gpuQueries_.shadow);
    gpuFrameStats_.depthMS = resolve(results, gpuQueries_.depth);
    gpuFrameStats_.lightCullMS = resolve(results, gpuQueries_.lightCull);
    gpuFrameStats_.geometryMS = resolve(results, gpuQueries_.geometry);
    gpuFrameStats_.aoMS = resolve(results, gpuQueries_.ao);
    gpuFrameStats_.postProcessMS = resolve(results, gpuQueries_.postProcess);

    cmd.ResetQueryPool(pool);
}

gfxapi::QueryPoolHandle GraphicsScene::QueryPool() const {
    return *queryPools_[state_.frameNumber % state_.framesInFlight];
}

void GraphicsScene::WaitFrameTasks() {
    PROFILE_EVENT_NC("Wait", COLOR_PROFILE_GRAPHICS);

    engine_.GetScheduler().Wait(schedulerGroup_);

    tasks_.ForEach([](auto& task) { task->~Task(); });
    tasks_.Clear();
}

} // namespace ugine