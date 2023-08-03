#include "ugine_material_bp.frag.hlsl"

BINDING(DATASET_MATERIAL, 0) cbuffer Params {
    float3 albedoColor;
    int albedoTexture;
    float3 emissiveColor;
    int emissiveTexture;
    int normalTexture;
    float alpha;
    float specularPower;
};

void material_bp_fragment(inout MaterialBpFragment material, float2 uv) {
    float a = alpha;

    material.albedo = albedoColor;
    
    if (albedoTexture >= 0) {
        // TODO: Sampler.
        float4 t = g_texture2d[albedoTexture].Sample(g_sampler[0], uv);
        material.albedo *= t.rgb;
        a *= t.a;
    }

    material.specularPower = specularPower;

#if defined(MATERIAL_MASKED)
    material.opacityMask = a;
#endif // MATERIAL_MASKED

#if defined(MATERIAL_OPACITY)
    material.opacity = a;
#endif // MATERIAL_OPACITY

    material.emissive = emissiveColor;
    if (emissiveTexture >= 0) {
        // TODO: Sampler.
        material.emissive *= g_texture2d[emissiveTexture].Sample(g_sampler[0], uv).rgb;
    }

    material.hasNormal = normalTexture > 0;
    if (normalTexture >= 0) {
        // TODO: Sampler.
        material.normal = 2.0f * g_texture2d[normalTexture].Sample(g_sampler[0], uv).rgb - 1.0f;
    }
}