#include "debug.hlsli"

struct VSInput {
    LOCATION(0) float3 position : POSITION0;
    LOCATION(1) float3 color : COLOR0;
};

PSInput main(VSInput input) {
    PSInput output;
    output.position = mul(params.viewProj, float4(input.position, 1.0));
    output.color = input.color;

    return output;
}
