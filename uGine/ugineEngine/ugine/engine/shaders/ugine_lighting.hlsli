#ifndef _UGINE_LIGHTING_H_
#define _UGINE_LIGHTING_H_

#include "Shader_Types.h"

struct LightingResult {
    float3 diffuse;
    float3 specular;
};

float CalcSpotCone(Light light, float3 lightDir) {
    // If the cosine angle of the light's direction
    // vector and the vector from the light source to the point being
    // shaded is less than minCos, then the spotlight contribution will be 0.
    float minCos = cos(light.spotAngleRad);
    // If the cosine angle of the light's direction vector
    // and the vector from the light source to the point being shaded
    // is greater than maxCos, then the spotlight contribution will be 1.
    float maxCos = lerp(minCos, 1., 0.5);
    float cosAngle = dot(light.direction, -lightDir);
    // Blend between the minimum and maximum cosine angles.
    return smoothstep(minCos, maxCos, cosAngle);
}

float DoAttenuation(Light light, float distance) {
    //return attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    return 1.0f - smoothstep(light.range * 0.75, light.range, distance);
}

#define LIGHT_ENABLED(light) ((light.typeEnabled & 0x1) > 0)
#define LIGHT_TYPE(light) ((light.typeEnabled & 0xfe) >> 1)

#endif // _UGINE_LIGHTING_H_