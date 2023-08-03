#include "Shader_Common.h"

PUSH_CONSTANT struct Params {
    float4 color;
    float4x4 mvp;
} params;

float4 main(in uint vertex
            : SV_VertexID)
    : SV_Position {
    float x = -1 + (vertex & 0x2);
    float y = -1 + 0.5 * (vertex & 0x4);
    float z = 1 * (vertex & 0x1);

    float3 position = float3(0.1 * x, 0.1 * y, 0.5 * z);

    return mul(params.mvp, float4(position, 1.0));
}
