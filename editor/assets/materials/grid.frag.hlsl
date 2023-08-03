#include "ugine_material_unlit.frag.hlsl"

#include "grid.hlsl"

void material_unlit_fragment(inout MaterialUnlitFragment material, float2 uv) {
    material.emissive = materialColor.rgb;
}