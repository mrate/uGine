#include "texture.hlsli"

PSOutput main(VSOutput input) {
    PSOutput pout;

#if !defined(PASS_DEPTH)
    if (textureID < 0 ) {
        pout.color = float4(1, 0, 1, 1);
    } else {
        pout.color = g_texture2d[textureID].Sample(g_sampler[0], input.inUV).rgba;
    }
#endif

    return pout;
}
