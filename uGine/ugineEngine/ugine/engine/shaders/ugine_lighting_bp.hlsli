#ifndef _UGINE_LIGHTING_BP_H_
#define _UGINE_LIGHTING_BP_H_

#include "ugine_surface.hlsli"

#if !defined(PASS_DEPTH)

float Diffuse(in Surface surface, in float3 lightDir) {
    return max(dot(surface.wsN, lightDir), 0.0);
}

float Specular(in Surface surface, in float3 lightDir) {
    float3 halfwayDir = normalize(lightDir + surface.wsV);
    return pow(max(dot(surface.wsN, halfwayDir), 0.0), surface.specularPower);
}

void CalcDirLight(in Surface surface, in Light light, inout LightingResult lighting) {
    float3 lightDir = light.direction;

    float diff = Diffuse(surface, lightDir);
    float spec = Specular(surface, lightDir);

    lighting.diffuse += light.color.rgb * diff * light.intensity;
    lighting.specular += light.color.rgb * spec * light.intensity;
}

void CalcPointLight(in Surface surface, in Light light, inout LightingResult lighting) {
    float3 lightDir = normalize(light.positionWS - surface.wsP);

    float diff = Diffuse(surface, lightDir);
    float spec = Specular(surface, lightDir);

    float distance = length(light.positionWS - surface.wsP);
    float attenuation = DoAttenuation(light, distance);

    lighting.diffuse += light.color.rgb * diff * attenuation * light.intensity;
    lighting.specular += light.color.rgb * spec * attenuation * light.intensity;
}

void CalcSpotLight(in Surface surface, in Light light, inout LightingResult lighting) {
    float3 lightDir = light.positionWS - surface.wsP;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);

    float attenuation = DoAttenuation(light, distance);
    float spotIntensity = CalcSpotCone(light, lightDir);

    float diff = Diffuse(surface, lightDir);
    float spec = Specular(surface, lightDir);

    lighting.diffuse += light.color.rgb * diff * attenuation * spotIntensity * light.intensity;
    lighting.specular += light.color.rgb * spec * attenuation * spotIntensity * light.intensity;
}

void LightingBP(in Surface surface, in uint lightOffset, in uint lightCount, inout LightingResult lighting) {
    float shadowStrength = 0.5f;

    // If this light generates shadow and fragment is not visible by this light then skip it.
    float visibility = 1.0;

    for (int i = 0; i < lightCount; ++i) {
        Light light = g_lights[g_lightIndex[lightOffset + i]];

        if (!LIGHT_ENABLED(light)) {
            continue;
        }

        switch (LIGHT_TYPE(light)) {
        case LIGHT_DIRECTION:
            if (light.shadowIndex >= 0 && light.shadowMapIndex >= 0) {
                //    int csm = min(CsmIndex(camera.position, surface.wsP), light.csmMaxLevel);
                visibility *= shadowStrength * CalcShadowFactorDir(
                    light.shadowMapIndex, mul(g_shadows[light.shadowIndex], float4(surface.wsP, 1.0)), float2(0, 0) /*, csm*/);
            }

            CalcDirLight(surface, light, lighting);
            break;
        case LIGHT_POINT:
            //if (light.shadowIndex >= 0) {
            //    shadowFactor += CalcShadowFactorPoint(light.shadowMapIndex, surface.wsP - light.position, light.range);
            //}

            CalcPointLight(surface, light, lighting);
            break;
        case LIGHT_SPOT:
            if (light.shadowIndex >= 0 && light.shadowMapIndex >= 0) {
                visibility
                    *= shadowStrength * CalcShadowFactorSpot(light.shadowMapIndex, mul(g_shadows[light.shadowIndex], float4(surface.wsP, 1.0)), /*light.range*/ float2(0, 0));
            }

            CalcSpotLight(surface, light, lighting);
            break;
        }
    }

    lighting.diffuse *= visibility;
    lighting.specular *= visibility;

    // Clamp specular highlights, otherwise it's just too bright and ugly...
    lighting.specular = clamp(lighting.specular, 0.0, 1.0);
}

#endif // !PASS_DEPTH

#endif // _UGINE_LIGHTING_BP_H_