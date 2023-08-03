#include "color.hlsli"

PSOutput main(VSOutput input) {
    PSOutput pout;

#if !defined(PASS_DEPTH)
    pout.color = materialColor;
#endif

    return pout;
}
