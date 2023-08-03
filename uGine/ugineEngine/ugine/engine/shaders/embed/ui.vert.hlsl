#include "ui.hlsli"

struct VSInput {
    LOCATION(0) float2 inPos : TEXCOORD0;
    LOCATION(1) float2 inUV : TEXCOORD1;
    LOCATION(2) float4 inColor : COLOR;
};

VSOutput main(VSInput input, out float4 pos : SV_Position) {
    VSOutput o;
    o.uv = input.inUV;
    //if ((params.flags & 1) != 0) {
    //    o.uv.y = 1.0 - o.uv.y;
    //}

    o.color = input.inColor;
    pos = float4(input.inPos * params.scale + params.translate, 0.0, 1.0);
    return o;
}