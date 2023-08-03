#include "ugine_material_unlit.frag.hlsl"

void material_unlit_fragment(inout MaterialUnlitFragment material, float2 uv) {
    material.emissive = float3(1, 0, 1);
}