#include "fullscreen.hlsli"

IMAGE_SAMPLER BINDING(0, 0) Texture2D inputTexture;
IMAGE_SAMPLER BINDING(0, 0) SamplerState inputSampler;

float4 main(VSOutput input)
    : SV_TARGET {
    return inputTexture.Sample(inputSampler, input.fragUV);
}