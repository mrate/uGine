#include "cubemap.hlsli"

VSOutput main(float3 position : POSITION0) {
    float4x4 view = g_camera.view;
    view[0][3] = 0;
    view[1][3] = 0;
    view[2][3] = 0;

    VSOutput vout;  
    vout.pos = mul(g_camera.proj, mul(view, mul(g_draw.model, float4(position, 1.0))));
    vout.pos = vout.pos.xyww; // Z=W - Always on the far plane.
    vout.inUV = position;
    return vout;
}
