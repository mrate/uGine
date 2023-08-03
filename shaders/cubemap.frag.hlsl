#include "cubemap.hlsli"

PSOutput main(VSOutput input) {
    PSOutput pout;

#if !defined(PASS_DEPTH)
    float3 color = float3(1, 0, 1);
    
    if (textureID >= 0) {
        color = g_textureCube[textureID].Sample(g_sampler[0], input.inUV).rgb;
    }

    pout.color = float4(color, 1.0);
#endif

    return pout;
}
