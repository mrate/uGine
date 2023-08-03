#include "Shader_Common.h"
#include "Shader_LightCull.h"

#include "ugine_global.hlsli"

struct VSOutput {
    LOCATION(0) float2 fragUV : TEXCOORD;
};

BINDING(0, 0) Texture2D<uint2> lightGrid;

PUSH_CONSTANT struct Params {
    uint2 resolution;
    uint lightsCount;
} params;

float4 main(VSOutput input, in float4 inPosition
            : SV_Position)
    : SV_Target {
    uint2 tileIndex = uint2(floor(inPosition.xy / LIGHT_CULL_BLOCK_SIZE));
    uint count = lightGrid[tileIndex].y;
    float3 temp = temperature(float(count) / float(params.lightsCount));

    return float4(sign(fmod(uint(inPosition.x), float(LIGHT_CULL_BLOCK_SIZE))) * sign(fmod(uint(inPosition.y), float(LIGHT_CULL_BLOCK_SIZE))) * temp, 1.0);
}