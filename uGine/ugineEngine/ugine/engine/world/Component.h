#pragma once

#include <ugine/engine/world/Camera.h>
#include <ugine/engine/world/Transformation.h>

#include <ugine/engine/gfx/Animation.h>
#include <ugine/engine/gfx/Model.h>

#include <gfxapi/Json.h>
#include <ugine/Json.h>

#include <array>

namespace ugine {

struct CameraComponent {
    bool isMain{};

    // TODO:
    Camera::ProjectionType projection{ Camera::ProjectionType::Perspective };
    f32 vFovDeg{ 70.0f };
    f32 zNear{ 0.01f };
    f32 zFar{ 100.0f };

    u32 width{ 128 };
    u32 height{ 128 };

    // TODO: Ortho

    // TODO: Remove.
    //Camera camera;

    //gfxapi::SampleCount samples{ SampleCount::e1 };
    //glm::mat4 viewProjectionPrev{};

    ColorRGBA clearColor{ 0, 0, 0, 1 };
    f32 clearDepth{ 1.0f };
    u8 clearStencil{ 0 };
};

// TODO:
inline glm::mat4 ProjectionMatrix(const CameraComponent& comp) {
    const auto camera{ [&] {
        switch (comp.projection) {
        case Camera::ProjectionType::Perspective: return Camera::Perspective(comp.vFovDeg, f32(comp.width), f32(comp.height), comp.zNear, comp.zFar);
        case Camera::ProjectionType::Ortho: return Camera::Ortho(0, 0, f32(comp.width), f32(comp.height), comp.zNear, comp.zFar);
        default: UGINE_ASSERT(false); return Camera{};
        }
    }() };

    return camera.ProjectionMatrix();
}

struct TransformationComponent {
    Transformation localTransformation{};
    Transformation globalTransformation{};
};

//
//struct ParticleComponent {
//    u32 count{ 1 };
//    gfx::TextureViewRef texture{};
//    gfx::Color color{};
//    glm::vec2 size{ 1.0f };
//    f32 life{ 1.0f };
//    f32 lifeSpan{ 0.1f };
//    f32 intensity{ 1.0f };
//    glm::vec3 gravity{};
//    glm::vec3 speed{};
//    glm::vec3 startOffset{};
//    glm::vec3 speedOffset{};
//    u32 emitCount{ 0 };
//    bool mesh{};
//
//    //
//    math::AABB aabb;
//};
//
//struct RigidBodyComponent {
//    enum class Shape {
//        Box,
//        Sphere,
//        Plane,
//    };
//
//    Shape shape{ Shape::Box };
//    glm::vec3 scale{ 1.0f };
//    glm::vec3 offset{ 0.0f };
//    glm::vec3 linearVelocity{};
//    glm::vec3 angularVelocity{};
//    glm::vec3 force{};
//    glm::vec3 torque{};
//    bool isDynamic{};
//    bool isKinematic{};
//
//    void AddForce(const glm::vec3& v) {
//        force += v;
//    }
//
//    void AddTorque(const glm::vec3& v) {
//        torque += v;
//    }
//};
//
//struct TerrainComponent {
//    std::filesystem::path heightMap;
//    gfx::TextureViewRef layers;
//    gfx::TextureViewRef splat;
//    glm::vec2 size{ 64, 64 };
//    f32 heightScale{ 1.0f };
//
//    //
//    math::AABB aabb;
//};
//
//struct FoliageComponent {
//    static constexpr int MAX_LAYERS{ 8 };
//
//    struct Layer {
//        u32 count;
//        gfx::FoliageRef foliage;
//        bool shadows{ false };
//    };
//
//    u32 layerCount{ 0 };
//    std::array<Layer, MAX_LAYERS> layers;
//
//    //
//    std::array<u32, MAX_LAYERS> realCount{};
//    math::AABB aabb;
//};

} // namespace ugine
