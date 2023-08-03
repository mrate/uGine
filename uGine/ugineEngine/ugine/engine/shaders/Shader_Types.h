#ifndef _SHADER_TYPES_H_
#define _SHADER_TYPES_H_

#include "Shader_Common.h"

// Consts.
#define LIGHT_DIRECTION 0
#define LIGHT_POINT 1
#define LIGHT_SPOT 2

UGINE_NAMESPACE_BEGIN

struct STRUCT_ALIGN IndirectDraw {
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};

struct STRUCT_ALIGN IndirectIndexedDraw {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

struct STRUCT_ALIGN Light {
    float3 color;
    float intensity;
    float3 positionWS;
    float spotAngleRad;
    float3 direction;
    float range;
    uint typeEnabled;
    int shadowIndex;
    int shadowMapIndex;
};

struct STRUCT_ALIGN Camera {
    float4x4 viewProj;
    float4x4 view;
    float4x4 proj;
    float4x4 invView;
    float4x4 invProj;
    float4x4 viewProjPrev;
    float3 position;
    float pad;
    float zNear;
    float zFar;
    float2 resolution;
    int aoTexture;
};

struct STRUCT_ALIGN AABB {
    float3 mmin;
    float3 mmax;
};

struct STRUCT_ALIGN Plane {
    float3 N; // Normal
    float d;  // Distance
};

struct STRUCT_ALIGN Frustum {
    Plane planes[4];
};

struct STRUCT_ALIGN Sphere {
    float3 c; // Center
    float r;  // Radius
};

struct STRUCT_ALIGN Cone {
    float3 T; // Tip
    float h;  // Height
    float3 d; // Direction
    float r;  // Bottom radius.
};

struct STRUCT_ALIGN Global {
    float time;
    uint lightCount;
};

struct STRUCT_ALIGN Draw {
    float4x4 model;
    float4x4 normal;
};

UGINE_NAMESPACE_END

#endif // _SHADER_TYPES_H_