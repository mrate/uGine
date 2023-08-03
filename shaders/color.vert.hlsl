#include "color.hlsli"

VSOutput main(float3 position : POSITION0) {
    VSOutput vout;
    vout.pos = mul(g_camera.viewProj, float4(position, 1.0));
    return vout;
}
