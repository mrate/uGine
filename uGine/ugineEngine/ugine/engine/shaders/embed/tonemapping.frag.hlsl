#include "Shader_Types.h"
#include "Shader_Tonemapping.h"

#include "ugine_global.hlsli"

BINDING(0, 0) Texture2D g_image;
BINDING(0, 1) SamplerState g_sampler;

UNIFORM_BUFFER(0, 2, Tonemapping, params);

struct VSOutput {
    LOCATION(0) float2 fragUV : TEXCOORD0;
};

float3 Reinhard2(float3 x) {
    const float L_white = 4.0;

    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

float3 Reinhard2(float x) {
    const float L_white = 4.0;

    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

float3 Uncharted2Tonemap(float3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float uncharted2Tonemap(float x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float uncharted2(float color) {
    const float W = 11.2;
    //const float exposureBias = 2.0;
    float exposureBias = params.exposure;
    float curr = uncharted2Tonemap(exposureBias * color);
    float whiteScale = 1.0 / uncharted2Tonemap(W);
    return curr * whiteScale;
}

float4 main(VSOutput input)
    : SV_Target {
    float3 hdrColor = g_image.Sample(g_sampler, input.fragUV).rgb;

    // exposure tone mapping
    float3 mapped = 0.0;
    switch (params.algorithm) {
    case 0: mapped = 1.0 - exp(-hdrColor * params.exposure); break;
    case 1:
        mapped = Uncharted2Tonemap(hdrColor);
        mapped = mapped * (1.0f / Uncharted2Tonemap(11.2f));
        break;
    }

    // Gamma correction
    mapped = pow(mapped, 1.0 / params.gamma);

    return float4(mapped, 1.0);
}