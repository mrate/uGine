#include "Shader_Common.h"
#include "Shader_UI.h"

struct VSOutput {
    LOCATION(0) float2 uv : TEXCOORD0;
    LOCATION(1) float4 color : COLOR;
    LOCATION(2) uint texID : BLENDINDICES0;
};
