#include "Shader_Common.h"

struct VertexInputType {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct PixelInputType {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

PixelInputType vs(VertexInputType input) {
    PixelInputType output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

float4 ps(PixelInputType input)
    : SV_Target {

    return float4(input.color, 1.0);
}
