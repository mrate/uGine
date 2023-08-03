#include "ugine_material_unlit.frag.hlsl"

#include "bone.hlsli"

void material_unlit_fragment(inout MaterialUnlitFragment material, float2 uv) {
    material.emissive = materialColor.rgb;

#ifdef MATERIAL_OPACITY
    material.opacity = materialColor.a;
#endif
}