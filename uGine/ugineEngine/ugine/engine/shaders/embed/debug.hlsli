#include "Shader_Common.h"

struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR0;
};

PUSH_CONSTANT struct Params {
    float4x4 viewProj;
} params;
