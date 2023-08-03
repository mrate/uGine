#include "Shader_SSAO.h"
#include "Shader_Types.h"

#include "ugine_global.hlsli"
#include "ugine_noise.hlsli"

// TODO: Bindless.
BINDING(0, 0) Texture2D g_depth;
BINDING(0, 1) SamplerState g_depthSampler;

UNIFORM_BUFFER(0, 2, Camera, g_camera);
UNIFORM_BUFFER(0, 3, Ssao, ssao);

struct VSOutput {
    LOCATION(0) float2 fragUV : TEXCOORD0;
};

float main(VSOutput input)
    : SV_Target {
    float depth = g_depth.Sample(g_depthSampler, input.fragUV).r;
    float3 positionVS = ViewSpaceFromDepth(input.fragUV, g_camera, depth);

    // TODO:
    float3 normalVS = NormalVSFromDepth(input.fragUV, g_camera, g_depth, g_depthSampler);

    // TODO: Random vector rotation.
    // TODO:
    float3 randomVec = float3(value_noise(input.fragUV), value_noise(1 - input.fragUV), 0);

    float3 tangentVS = normalize(randomVec - normalVS * dot(randomVec, normalVS));
    float3 bitangentVS = cross(normalVS, tangentVS);
    float3x3 TBN = float3x3(tangentVS, bitangentVS, normalVS);

    float ao = 0.0;
    for (int i = 0; i < UGINE_SSAO_KERNEL_SIZE; ++i) {
        // Sample position - fragment position offset by radnom hemisphere vector * sample radius.
        float3 samplePosVS = mul(ssao.samples[i].xyz, TBN); // Tangent to VS.
        samplePosVS = positionVS + samplePosVS * ssao.radius;

        // VS to clip space.
        float4 offset = float4(samplePosVS, 1.0);
        offset = mul(g_camera.proj, offset); // from view to clip-space
        offset.xyz /= offset.w;              // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        // TODO: Sample linear depth texture.
        depth = g_depth.Sample(g_depthSampler, offset.xy).r;
        float sampleDepth = -ViewSpaceFromDepth(offset.xy, g_camera, depth).z;

        float rangeCheck = smoothstep(0.0, 1.0, ssao.radius / abs(-positionVS.z - sampleDepth));

        ao += (sampleDepth <= -samplePosVS.z + ssao.bias ? 1 : 0) * rangeCheck;
    }
    ao = 1 - ssao.strength * (ao / float(UGINE_SSAO_KERNEL_SIZE));

    return ao;
}