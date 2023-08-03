#include "Shader_Common.h"

#include "common/Capsule.hlsl"

PUSH_CONSTANT struct Params {
    float4 color;
    float4x4 mvp;
} params;

float4 main(in uint vertex
            : SV_VertexID)
    : SV_Position {
    return mul(params.mvp, CAPSULE[vertex]);
}
