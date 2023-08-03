#include "Shader_Common.h"

#include "Bindings_Material.hlsl"

#include "ugine_material_vertex.hlsl"

struct VSOutput {
    LOCATION(0) float4 pos : SV_POSITION;
};

struct PSOutput {
#if !defined(PASS_DEPTH)
    LOCATION(0) float4 color : SV_Target;
#endif
};

BINDING(DATASET_MATERIAL, 0) cbuffer Params {
    float4 materialColor;
};
