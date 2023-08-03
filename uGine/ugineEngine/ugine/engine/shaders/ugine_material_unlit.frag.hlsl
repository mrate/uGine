#include "Shader_Common.h"

#include "Bindings_Material.hlsl"

#include "ugine_material_unlit.hlsl"

struct VSOutput {
    LOCATION(0) float2 inUV : TEXCOORD0;
};

struct PSOutput {
#if !defined(PASS_DEPTH)
    LOCATION(0) float4 color : SV_Target;
#endif
};

// Implement:
void material_unlit_fragment(inout MaterialUnlitFragment material, float2 uv);

PSOutput main(VSOutput input) {
    MaterialUnlitFragment material;
    material.emissive = 0.0f;
#if defined(MATERIAL_MASKED)
    material.opacityMask = 1;
#endif // MATERIAL_MASKED

    material_unlit_fragment(material, input.inUV);

#if defined(MATERIAL_MASKED)
    if (material.opacityMask < 0.5f) {
        discard;
    }
#endif // MATERIAL_MASKED

    PSOutput output;
#if !defined(PASS_DEPTH)

#ifdef MATERIAL_OPACITY
    float alpha = material.opacity;
#else  // MATERIAL_OPACITY
    float alpha = 1.0f;
#endif // MATERIAL_OPACITY

    output.color = float4(material.emissive, alpha);
#endif // !PASS_DEPTH
    return output;
}
