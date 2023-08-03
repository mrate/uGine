#include "Shader_Common.h"

struct VertexInputType {
    float3 position : POSITION;
    float2 uv : TEXCOORD;
};

struct PixelInputType {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

BINDING(0, 0) Texture2D BindlessTextures[];
BINDING(1, 0) SamplerState BindlessSamplers[];

PUSH_CONSTANT struct Params {
    uint textureIndex;
    uint samplerIndex;
} params;

PixelInputType vs(VertexInputType input) {
    PixelInputType output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    return output;
}

float4 ps(PixelInputType input)
    : SV_Target {
    return BindlessTextures[params.textureIndex].Sample(BindlessSamplers[params.samplerIndex], input.uv).rgba;
}
