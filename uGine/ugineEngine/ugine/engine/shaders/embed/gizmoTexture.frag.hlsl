#include "Shader_Common.h"

IMAGE_SAMPLER BINDING(0, 0) Texture2D inputTexture;
IMAGE_SAMPLER BINDING(0, 0) SamplerState inputSampler;

struct VSOutput {
    LOCATION(0) float2 fragUV;
    LOCATION(1) float4 color;
}

float4
main(VSOutput input)
    : SV_Target {
    float4 color = inputTexture.Sample(inputSampler, input.fragUV);
    return mul(color, input.color);
}