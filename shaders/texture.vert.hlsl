#include "texture.hlsli"

VSOutput main(VSInput input) {
    VSOutput vout;
    vout.pos = mul(g_camera.viewProj, float4(input.position, 1.0));
    vout.inUV = input.texcoord;
    return vout;
}
