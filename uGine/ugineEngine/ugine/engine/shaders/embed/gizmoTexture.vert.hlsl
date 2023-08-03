#include "Shader_Common.h"

static int index[6] = { 0, 1, 2, 1, 3, 2 };

static float2 positions[4] = { { -1.0, -1.0 }, { 1.0, -1.0 }, { -1.0, 1.0 }, { 1.0, 1.0 } };
static float2 coords[4] = { float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(1.0, 1.0) };

struct VSOutput {
    LOCATION(0) float2 fragUV : TEXCOORD0;
    LOCATION(1) float4 color : COLOR;
};

PUSH_CONSTANT struct Params {
    float4x4 mvp;
    float4 color;
    float2 size;
} params;

VSOutput main(in uint vertex : SV_VertexID, out float4 position : SV_Position) {
    int i = index[vertex];

    float4 pos = float4(params.size * positions[i], 0.0, 1.0);

    VSOutput o;
    o.fragUV = coords[i];
    o.color = params.color;
    position = mul(params.mvp, pos);
    return o;
}