#include "Shader_Common.h"

struct VSInput {
    LOCATION(0) float3 position : POSITION0;
    LOCATION(1) float3 normal : NORMAL0;
    LOCATION(2) float3 tangent : TANGENT0;
    LOCATION(4) float2 texcoord : TEXCOORD0;

#if defined(MATERIAL_INSTANCE)
    LOCATION(5) float4 instance0 : POSITION1;
    LOCATION(6) float4 instance1 : POSITION2;
    LOCATION(7) float4 instance2 : POSITION3;
#endif // MATERIAL_INSTANCE
};

// Implement:
float3 material_vertex_offset();
