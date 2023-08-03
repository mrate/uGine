#include "Shader_Common.h"

PUSH_CONSTANT struct Line {
    float4 color;
    float4x4 viewProj;
    float3 from;
    float3 to;
} params;

float4 main(in uint vertex
            : SV_VertexID)
    : SV_Position {
    return mul(params.viewProj, float4(vertex == 0 ? params.from : params.to, 1.0));
}
