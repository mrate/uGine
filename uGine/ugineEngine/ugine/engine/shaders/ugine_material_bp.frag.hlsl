#include "Shader_Common.h"

#include "Bindings_Material.hlsl"

#include "ugine_material_bp.hlsl"
#include "ugine_shadows.hlsli"

#include "ugine_lighting.hlsli"
#include "ugine_lighting_bp.hlsli"

#include "Shader_LightCull.h"

// Implement:
void material_bp_fragment(inout MaterialBpFragment material, float2 uv);

struct VSOutput {
    LOCATION(0) float2 fragUV : TEXCOORD0;
    LOCATION(1) float3 positionVS : POSITION0;

#if !defined(PASS_DEPTH)
    LOCATION(2) float3 tangentWS : TANGENT0;
    LOCATION(3) float3 bitangentWS : BINORMAL0;
    LOCATION(4) float3 normalWS : NORMAL0;
    LOCATION(5) float3 positionWS : POSITION1;
    LOCATION(6) float3 positionM : POSITION2;
#endif
};

struct PSOutput {
#if !defined(PASS_DEPTH)
    float4 color : SV_Target;
#endif
};

PSOutput main(VSOutput input, in float4 inPosition : SV_Position) {
    PSOutput output;

    float2 texCoords = input.fragUV;

#if !defined(PASS_DEPTH)
    Surface surface;
    surface.wsP = input.positionWS;
    surface.wsV = normalize(g_camera.position - input.positionWS);
    surface.wsN = normalize(input.normalWS);

#if defined(MATERIAL_HEIGHT_MAP)
    //if (material.hasHeightTexture > 0 || material.hasNormalWithHeight != 0) {
    //    mat3 TBN = transpose(mat3(inTangentWS, inBitangentWS, inNormalWS));
    //    float3 tangentView = TBN * g_camera.position;
    //    float3 tangentFragPos = TBN * inPositionWS;
    //
    //    float3 tangentViewDir = normalize(tangentView - tangentFragPos);
    //
    //    texCoords = Parallax(texCoords, tangentViewDir, material.heightScale, normalTexture, heightTexture, material.hasNormalWithHeight);
    //    if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) {
    //        discard;
    //    }
    //}
#endif // MATERIAL_HEIGHT_MAP
#endif // !PASS_DEPTH

    MaterialBpFragment material;
    material.normal = 0.0f;
    material.hasNormal = false;
    material.albedo = 0.0f;
    material.specularPower = 1;
    material.specular = 0.0f;
    material.emissive = 0.0f;

#if defined(MATERIAL_MASKED)
    material.opacityMask = 1;
#endif // MATERIAL_MASKED

#if defined(MATERIAL_OPACITY)
    material.opacity = 1;
#endif // MATERIAL_OPACITY

    material_bp_fragment(material, input.fragUV);

#if defined(MATERIAL_MASKED)
    if (material.opacityMask < 0.5f) {
        discard;
    }
#endif // MATERIAL_MASKED

#if !defined(PASS_DEPTH)

#if defined(MATERIAL_OPACITY)
    float alpha = material.opacity;
#else  // MATERIAL_OPACITY
    float alpha = 1.0f;
#endif // MATERIAL_OPACITY

    if (material.hasNormal) {
        float3x3 TBN = float3x3(normalize(input.tangentWS), normalize(input.bitangentWS), surface.wsN);
        surface.wsN = normalize(float4(mul(material.normal, TBN), 0)).xyz;
    }

    float3 diffuse = material.albedo;

    float2 screenSpace = inPosition.xy / g_camera.resolution;
    // TODO: Sampler.
    float ao = 1;
    if (g_camera.aoTexture >= 0) {
        ao = g_texture2d[g_camera.aoTexture].Sample(g_sampler[0], screenSpace).r;
    }

    float3 ambient = 0.1f;

    // TODO:
    uint2 tileIndex = uint2(floor(inPosition.xy / LIGHT_CULL_BLOCK_SIZE));
    uint2 lightOffsetCount = g_lightGrid[tileIndex].xy;

    surface.albedo = material.albedo;
    surface.emissiveColor = material.emissive;
    surface.specularPower = material.specularPower;

    LightingResult lighting;
    lighting.diffuse = 0;
    lighting.specular = 0;

    LightingBP(surface, lightOffsetCount.x, lightOffsetCount.y, lighting);

    diffuse *= lighting.diffuse;

    float3 specular = 0.0f;
    if (surface.specularPower > 1.0f) {
        // If specular power is too low, don't use it.
        //specular = surface.specular * lighting.specular;
        specular = lighting.specular;
    }

    float3 color = surface.emissiveColor + ao * (ambient + (diffuse + specular));
    output.color = float4(color.rgb, alpha);
    //output.color = float4(0.5 + 0.5 * surface.wsN, 1.0);
#endif // !PASS_DEPTH

    return output;
}
