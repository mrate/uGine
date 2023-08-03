#include "debug.hlsli"

float4 main(PSInput input)
    : SV_Target {
    return float4(input.color, 1.0f);
}