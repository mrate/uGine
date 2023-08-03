#include "ui.hlsli"

// TODO:
BINDING(0, 0) Texture2D images2d[];

BINDING(1, 0) SamplerState samplers[];

float4 main(VSOutput input)
    : SV_Target {

    float4 textureColor = 0;
    if (params.flags == 0) {
        textureColor = images2d[params.textureSampler.x].Sample(samplers[params.textureSampler.y], input.uv);
    } else {
        float depth = images2d[params.textureSampler.x].Sample(samplers[params.textureSampler.y], input.uv).r;
        textureColor = float4(depth, depth, depth, 1);
    }

    return input.color * textureColor;
}