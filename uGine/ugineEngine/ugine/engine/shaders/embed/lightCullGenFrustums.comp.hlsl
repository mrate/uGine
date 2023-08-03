#include "Shader_Common.h"

#include "ugine_global.hlsli"

#include "Shader_LightCull.h"
#include "Shader_LightCullFrustums.h"

BINDING(0, 0) RWStructuredBuffer<Frustum> frustums;

// Convert clip space coordinates to view space
float4 ClipToView(float4 clip) {
    // View space position.
    float4 view = mul(params.inverseProjection, clip);
    // Perspective projection.
    view = view / view.w;
    return view;
}

// Convert screen space coordinates to view space.
float4 ScreenToView(float4 screen) {
    // Convert to normalized texture coordinates
    float2 texCoord = screen.xy / float2(params.screenResolution);

    // Convert to clip space
    float4 clip = float4(float2(texCoord.x, 1.0 - texCoord.y) * 2.0 - 1.0, screen.z, screen.w);
    return ClipToView(clip);
}

// https://www.3dgep.com/forward-plus/
[numthreads(LIGHT_CULL_BLOCK_SIZE, LIGHT_CULL_BLOCK_SIZE, 1)] void main(in uint3 dispatchThreadId
                                                                        : SV_DispatchThreadID) {
    uint2 index = dispatchThreadId.xy;

    float3 eyePos = 0.0f;
    float z = -1.0;

    float4 screenSpace[4];
    screenSpace[0] = float4(index.xy * LIGHT_CULL_BLOCK_SIZE, z, 1.0);
    screenSpace[1] = float4(float2(index.x + 1, index.y) * LIGHT_CULL_BLOCK_SIZE, z, 1.0);
    screenSpace[2] = float4(float2(index.x, index.y + 1) * LIGHT_CULL_BLOCK_SIZE, z, 1.0);
    screenSpace[3] = float4(float2(index.x + 1, index.y + 1) * LIGHT_CULL_BLOCK_SIZE, z, 1.0);

    float3 viewSpace[4];
    viewSpace[0] = ScreenToView(screenSpace[0]).xyz;
    viewSpace[1] = ScreenToView(screenSpace[1]).xyz;
    viewSpace[2] = ScreenToView(screenSpace[2]).xyz;
    viewSpace[3] = ScreenToView(screenSpace[3]).xyz;

    Frustum frustum;
    frustum.planes[0] = ComputePlane(eyePos, viewSpace[2], viewSpace[0]);
    frustum.planes[1] = ComputePlane(eyePos, viewSpace[1], viewSpace[3]);
    frustum.planes[2] = ComputePlane(eyePos, viewSpace[0], viewSpace[1]);
    frustum.planes[3] = ComputePlane(eyePos, viewSpace[3], viewSpace[2]);

    if (index.x < params.dispatchResolution.x && index.y < params.dispatchResolution.y) {
        uint frustumIndex = index.x + index.y * params.dispatchResolution.x;
        frustums[frustumIndex] = frustum;
    }
}
