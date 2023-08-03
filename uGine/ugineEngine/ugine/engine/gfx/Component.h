#pragma once

#include <gfxapi/Json.h>
#include <ugine/Json.h>

#include "Animation.h"
#include "Model.h"

namespace ugine {

struct MeshComponent {
    ModelInstance modelInstance;
    bool instanced{};
    std::vector<MaterialVertexInstance> instanceTransformations;
};

struct AnimationControllerComponent {
    ResourceHandle<Animation> animation;
    bool isRunning{ true };
    f32 speed{ 1.0f };
    f32 resolutionS{ 0.03f }; // 30ms ~ 30fps.
    f32 animationTime{};
};

struct LightComponent {
    enum class Type {
        Directional,
        Point,
        Spot,
    };

    Type type{ Type::Directional };
    f32 intensity{ 1.0f };
    ColorRGB color{ 1.0f, 1.0f, 1.0f };
    f32 range{ 10.0f };
    f32 spotAngleDeg{ 45.0f };

    bool generatesShadows{};

    ResourceHandle<Texture> flareTexture;
    glm::vec2 flareSize{ 0.25f, 0.25f };
    f32 flareFade{ 0.001f };
};

struct SkyComponent {
    ResourceHandle<Material> material;
};

} // namespace ugine