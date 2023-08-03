#include "Shader_Common.h"

PUSH_CONSTANT struct ViewProj {
    float4 color;
    float4x4 mvp;
    float vfovTanHalf;
    float hfovTanHalf;
    float near;
    float far;
} params;

float4 main(in uint vertex
            : SV_VertexID)
    : SV_Position {
    float3 position = float3(params.hfovTanHalf, params.vfovTanHalf, -1);

    switch (vertex) {
    case 0:
    case 7:
    case 16: position *= params.near; break;
    case 1:
    case 2:
    case 18:
        position *= params.near;
        position.x *= -1;
        break;
    case 3:
    case 4:
    case 20:
        position *= params.near;
        position.x *= -1;
        position.y *= -1;
        break;
    case 5:
    case 6:
    case 22:
        position *= params.near;
        position.y *= -1;
        break;
    case 8:
    case 15:
    case 17: position *= params.far; break;
    case 9:
    case 10:
    case 19:
        position *= params.far;
        position.x *= -1;
        break;
    case 11:
    case 12:
    case 21:
        position *= params.far;
        position.x *= -1;
        position.y *= -1;
        break;
    case 13:
    case 14:
    case 23:
        position *= params.far;
        position.y *= -1;
        break;
    }

    return mul(params.mvp, float4(position, 1.0));
    //outPosition = gl_Position;
}
