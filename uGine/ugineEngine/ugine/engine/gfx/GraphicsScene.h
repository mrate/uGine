#pragma once

#include <ugine/engine/engine/Engine.h>
#include <ugine/engine/gfx/Component.h>
#include <ugine/engine/gfx/GpuQuery.h>
#include <ugine/engine/gfx/RenderContext.h>
#include <ugine/engine/gfx/helpers/DebugRenderer.h>
#include <ugine/engine/math/Aabb.h>
#include <ugine/engine/math/Raycast.h>
#include <ugine/engine/world/Camera.h>
#include <ugine/engine/world/Component.h>
#include <ugine/engine/world/GameObject.h>
#include <ugine/engine/world/World.h>

#include <ugine/Scheduler.h>
#include <ugine/Vector.h>

#include <entt/entt.hpp>

#include <glm/glm.hpp>

namespace ugine {

class World;
class GraphicsState;

struct LightShaderData;
struct LightRenderData;
struct CameraRenderData;
struct AnimatorRenderData;
struct InstanceRenderData;
struct SkyRenderData;

struct VisibilityList {
    enum Flags : u32 {
        FLAG_MESHES = UGINE_BIT(0),
        FLAG_LIGHTS = UGINE_BIT(1),

        ALL = u32(-1),
    };

    u32 flags{ ALL };
    u32 layerMask{ u32(-1) };

    u32 drawCalls{};
    Vector<GameObjectHandle> meshes;
    Vector<GameObjectHandle> lights;

    void Init() {
        drawCalls = 0;
        meshes.Clear();
        lights.Clear();
    }
};

struct ParallelCull {
    struct Result {
        u32 drawCalls{};
        Vector<GameObjectHandle> meshes;
        Vector<GameObjectHandle> lights;
    };

    Frustum frustum;
    std::array<Result, UGINE_MAX_THREADS> perThreadResult;

    Scheduler::Group* group{};
    VisibilityList* visibility{};
};

struct ParallelRayCast {
    Ray ray;
    Scheduler::Group group;
    std::array<WorldHit, UGINE_MAX_THREADS> perThreadResult;
    std::array<u32, UGINE_MAX_THREADS> perThreadComponentCount;
};

class GraphicsScene final : public WorldScene {
public:
    inline static const auto NAME{ "GraphicsScene"_hs };

    struct FrameStats {
        u32 drawCalls{};
        u32 computeDispatches{};
    };

    struct FrameGpuStats {
        float shadowsMS{};
        float depthMS{};
        float lightCullMS{};
        float geometryMS{};
        float aoMS{};
        float postProcessMS{};
    };

    GraphicsScene(Engine& engine, World& world, GraphicsState& state, IAllocator& allocator = IAllocator::Default());
    ~GraphicsScene();

    // WorldScene::*
    WorldHit RayCast(const Ray& ray) const override;
    StringID Name() const override { return NAME; }
    void Update() override;

    // GraphicsScene::*
    void Render(gfxapi::CommandList& cmd);
    void RenderCamera(gfxapi::CommandList& cmd, const GameObject& cameraGO);
    void RenderShadow(gfxapi::CommandList& cmd, const GameObject& shadowCaster);

    const FrameStats& GetFrameStats() const { return frameStats_; }
    const FrameGpuStats& GetFrameGpuStats() const { return gpuFrameStats_; }

    DebugRenderer& GetDebugRenderer() { return debugRenderer_; }
    Camera LightCamera(const LightComponent& shadowCaster, const gfxapi::Extent2D& resolution) const;

    void CopyLightData(void* dst) const;
    size_t LightDataSize() const;

    Vector<Draw> GetDrawList(const VisibilityList& visibility) const;

    gfxapi::TextureHandle GetCameraRtv(const GameObject& go) const;
    void SetCameraRtv(const GameObject& go, gfxapi::TextureHandle output, const gfxapi::Extent2D& extent);
    gfxapi::TextureHandle GetCameraDepthBuffer(const GameObject& go) const;
    int GetAoTextureIndex(const GameObject& go) const;

    gfxapi::TextureHandle GetShadowMap(const GameObject& go) const;
    const glm::mat4& GetLightCameraProjection(const GameObject& go) const;
    const glm::mat4& GetLightCameraView(const GameObject& go) const;

private:
    struct GpuQueries {
        // TODO: Multiple shadows, multiple geometries (cameras).
        GpuQuery shadow{};
        GpuQuery depth{};
        GpuQuery lightCull{};
        GpuQuery geometry{};
        GpuQuery ao{};
        GpuQuery postProcess{};
    };

    void Cull(ParallelCull& cull, u32 flags) ;
    void StoreCullResults(ParallelCull& cull) const;

    void WaitUpdate();

    void UpdateLights();
    void UpdateLightShaderData(LightShaderData& shaderData, const LightComponent& light) const;
    void UpdateLightRenderData(LightRenderData& renderData, const LightComponent& light, const Transformation& transformation) const;
    void UpdateLightTransformation(LightShaderData& shaderData, const Transformation& transformation) const;
    void UpdateLightAabb(LightRenderData& aabb, const LightComponent& light, const Transformation& transformation) const;
    void LightShaderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) const;
    void LightRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) const;

    void UpdateCameras();
    void UpdateCamera(CameraRenderData& cameraRD, const CameraComponent& camera, const Transformation& transformation) const;
    void UpdateCameraFrustums(gfxapi::CommandList& cmd, CameraRenderData& data) const;
    void CameraRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent) const;

    void InitAnimatorRenderData(const ModelInstance& model, AnimatorRenderData& renderData) const;

    void UpdatePendingAnimations();
    void UpdateAnimationControllers();
    void UpdateAnimations(Scheduler::Group& group, f64 frameSeconds);
    void UploadAnimationRenderData(gfxapi::CommandList& cmd);
    void AnimatorRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent);
    void AnimatorRenderDataDestroyed(GameObjectRegistry& reg, GameObjectHandle ent);

    void UpdateMeshes();
    void UpdatePendingMeshes();
    void MeshCreated(GameObjectRegistry& reg, GameObjectHandle ent);
    void MeshDestroyed(GameObjectRegistry& reg, GameObjectHandle ent);
    void InstancedRenderDataCreated(GameObjectRegistry& reg, GameObjectHandle ent);
    void InstancedRenderDataDestroyed(GameObjectRegistry& reg, GameObjectHandle ent);
    void UpdateMeshInstancedData(InstanceRenderData& renderData, const MeshComponent& mesh);
    void UpdateMeshAabb(GameObject& go) const;

    void MeshModelReady(GameObject& go) const;

    void AddMeshDraw(Vector<Draw>& draws, GameObjectHandle handle) const;
    void CullMeshes(ParallelCull& cull) ;

    void AddSkyDraw(Vector<Draw>& draws, const SkyRenderData& renderData) const;
    void SkyCreated(GameObjectRegistry& reg, GameObjectHandle ent);

    void MeshRayCast(ParallelRayCast& rayCast, GameObjectHandle go) const;

    static shaders::Camera CameraShaderData(const Camera& camera, const glm::mat4& inverseViewMatrix, const glm::vec3& position);

    void PrepareRenderData(gfxapi::CommandList& cmd);
    void UpdateGpuStats(gfxapi::CommandList& cmd);
    gfxapi::QueryPoolHandle QueryPool() const;

    template <typename T, typename... Args> T* NewFrameTask(Args&&... args) {
        auto task{ engine_.FrameAllocator().AlignedNew<T>(std::forward<Args>(args)...) };
        tasks_.PushBack(task);
        return task;
    }

    //template <typename F> void RunFrameTask(Scheduler::Group& grp, u32 num, F f) {
    //    template <typename L>
    //    struct LambdaTask : public Task {
    //        LambdaTask(u32 num, L lambda)
    //            : Task{ num }
    //            , lambda{ lambda } {}

    //        void Run(u32 start, u32 end, u32 threadNum) override { 
    //            lambda(start, end, threadNum);
    //        }

    //        L lambda;
    //    };

    //    NewFrameTask(grp, )
    //}

    void WaitFrameTasks();

    GraphicsState& state_;
    AllocatorRef allocator_;
    mutable DebugRenderer debugRenderer_; // TODO:

    entt::observer updatedLightTransformations_{};
    entt::observer updatedLights_{};
    Vector<GameObject> deletedLights_;

    entt::observer updatedCameras_{};
    entt::observer translatedCameras_{};
    Vector<GameObject> deletedCameras_;

    entt::observer updatedMeshes_;
    entt::observer translatedMeshes_;
    Vector<GameObject> deletedMeshes_;

    entt::observer updatedAnimationControllers_;

    Vector<glm::mat4> shadowMatrices_;

    // Frame render data.
    shaders::Global global_{};
    gfxapi::GpuAllocation gpuGlobal_{};
    gfxapi::GpuAllocation gpuLightsSB_{};
    gfxapi::GpuAllocation gpuShadowsSB_{};

    // Stats.
    FrameStats frameStats_{};
    FrameGpuStats gpuFrameStats_{};

    Vector<gfxapi::QueryPoolHandleUnique> queryPools_;
    GpuQueries gpuQueries_{};

    // Tasks
    Scheduler::Group schedulerGroup_{};
    Vector<Task*> tasks_;

    // TODO:
    mutable u32 meshesCnt_{};
};

} // namespace ugine