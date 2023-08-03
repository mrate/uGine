#include "Shader_Common.h"

#include "ugine_global.hlsli"
#include "ugine_lighting.hlsli"

#include "Shader_LightCull.h"

// TODO: Bindless.
BINDING(0, 0) Texture2D depthVS;

BINDING(0, 1) RWTexture2D<uint2> o_LightGrid;
BINDING(0, 2) RWTexture2D<uint2> t_LightGrid;

BINDING(0, 3) RWStructuredBuffer<uint> o_lightIndexCounter;
BINDING(0, 4) RWStructuredBuffer<uint> t_lightIndexCounter;
BINDING(0, 5) RWStructuredBuffer<uint> o_lightIndexList;
BINDING(0, 6) RWStructuredBuffer<uint> t_lightIndexList;
BINDING(0, 7) StructuredBuffer<Light> in_lights;
BINDING(0, 8) StructuredBuffer<Frustum> in_frustums;

UNIFORM_BUFFER(0, 9, LightCullParams, params);

groupshared uint o_LightCount;
groupshared uint o_LightIndexStartOffset;
groupshared uint o_LightList[LIGHT_LIST_SIZE];

groupshared uint t_LightCount;
groupshared uint t_LightIndexStartOffset;
groupshared uint t_LightList[LIGHT_LIST_SIZE];

groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared Frustum groupFrustum;

void o_AppendLight(uint lightIndex) {
    uint index;
    InterlockedAdd(o_LightCount, 1, index);
    if (index < LIGHT_LIST_SIZE) {
        o_LightList[index] = lightIndex;
    }
}

void t_AppendLight(uint lightIndex) {
    uint index;
    InterlockedAdd(t_LightCount, 1, index);
    if (index < LIGHT_LIST_SIZE) {
        t_LightList[index] = lightIndex;
    }
}

// Convert clip space coordinates to view space
float4 ClipToView(float4 clip) {
    // View space position.
    float4 view = mul(params.inverseProjection, clip);
    // Perspective projection.
    view = view / view.w;
    return view;
}

struct CSInput {
    uint3 groupID : SV_GroupID;
    uint3 groupThreadID : SV_GroupThreadID;
    uint3 dispatchThreadID : SV_DispatchThreadID;
    uint groupIndex : SV_GroupIndex;
};

// https://www.3dgep.com/forward-plus/
[numthreads(LIGHT_CULL_BLOCK_SIZE, LIGHT_CULL_BLOCK_SIZE, 1)] void main(CSInput input) {
    int2 texCoord = input.dispatchThreadID.xy;
    float fDepth = depthVS.Load(int3(texCoord, 0)).r;
    uint uDepth = asuint(fDepth);

    if (input.groupIndex == 0) {
        uMinDepth = 0xffffffff;
        uMaxDepth = 0;

        o_LightCount = 0;
        t_LightCount = 0;

        groupFrustum = in_frustums[input.groupID.y * params.numThreadGroups.x + input.groupID.x];
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(uMinDepth, uDepth);
    InterlockedMax(uMaxDepth, uDepth);

    GroupMemoryBarrierWithGroupSync();

    float fMinDepth = asfloat(uMinDepth);
    float fMaxDepth = asfloat(uMaxDepth);

    float minDepthVS = ClipToView(float4(0, 0, fMinDepth, 1)).z;
    float maxDepthVS = ClipToView(float4(0, 0, fMaxDepth, 1)).z;
    float nearClipVS = ClipToView(float4(0, 0, 0, 1)).z;

    Plane minPlane = { float3(0, 0, -1), -minDepthVS };

    for (uint i = input.groupIndex; i < params.lightsCount; i += LIGHT_CULL_BLOCK_SIZE * LIGHT_CULL_BLOCK_SIZE) {
        Light light = in_lights[i];

        if (!LIGHT_ENABLED(light)) {
            continue;
        }

        float4 lightPosVS = mul(params.view, float4(light.positionWS, 1.0));

        switch (LIGHT_TYPE(light)) {
        case LIGHT_DIRECTION:
            t_AppendLight(i);
            o_AppendLight(i);
            break;
        case LIGHT_POINT: {
            Sphere sphere = { lightPosVS.xyz, light.range };
            if (SphereInsideFrustum(sphere, groupFrustum, nearClipVS, maxDepthVS)) {
                t_AppendLight(i);
                if (!SphereInsidePlane(sphere, minPlane)) {
                    o_AppendLight(i);
                }
            }
        } break;
        case LIGHT_SPOT: {
            float4 lightDirVS = normalize(mul(params.view, float4(light.direction, 0.0)));

            float coneRadius = tan(light.spotAngleRad) * light.range;
            Cone cone = { lightPosVS.xyz, light.range, lightDirVS.xyz, coneRadius };
            if (ConeInsideFrustum(cone, groupFrustum, nearClipVS, maxDepthVS)) {
                t_AppendLight(i);
                if (!ConeInsidePlane(cone, minPlane)) {
                    o_AppendLight(i);
                }
            }
        } break;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (input.groupIndex == 0) {
        InterlockedAdd(o_lightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);
        o_LightGrid[input.groupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);

        InterlockedAdd(t_lightIndexCounter[0], t_LightCount, t_LightIndexStartOffset);
        t_LightGrid[input.groupID.xy] = uint2(t_LightIndexStartOffset, t_LightCount);
    }

    GroupMemoryBarrierWithGroupSync();

    for (i = input.groupIndex; i < o_LightCount; i += LIGHT_CULL_BLOCK_SIZE * LIGHT_CULL_BLOCK_SIZE) {
        o_lightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
    }

    for (i = input.groupIndex; i < t_LightCount; i += LIGHT_CULL_BLOCK_SIZE * LIGHT_CULL_BLOCK_SIZE) {
        t_lightIndexList[t_LightIndexStartOffset + i] = t_LightList[i];
    }
}
