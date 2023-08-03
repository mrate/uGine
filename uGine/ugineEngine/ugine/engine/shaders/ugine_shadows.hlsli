#if !defined(PASS_DEPTH)

// Pridejte, nebojte se...
#define EPSILON_D 0
//.001

float CalcShadowFactorDir(int shadowMapIndex, float4 lightSpacePos, float2 off /*, int csm*/) {
    float visibility = 1.0;

    float3 shadowCoords = lightSpacePos.xyz / lightSpacePos.w;
    if (shadowCoords.z > -1.0 && shadowCoords.z < 1.0) {
        float2 normCoords = 0.5 * shadowCoords.xy + 0.5;
        float2 uv = normCoords + off;

        // TODO: Depth sampler.
        float depth = g_texture2d[shadowMapIndex].Sample(g_sampler[0], /*vec3(uv, csm)*/ uv).r;

        if (depth + EPSILON_D >= shadowCoords.z) {
            visibility = 0.0;
        }
    }

    return visibility;
}

float CalcShadowFactorPCFDir(int shadowMapIndex, float4 lightSpacePos /*, int csm */) {
    //    int2 texDim = textureSize(g_shadowMaps[shadowMapIndex], 0).xy;
    //    float scale = 1.0; // 1.5
    //    float dx = scale * 1.0 / float(texDim.x);
    //    float dy = scale * 1.0 / float(texDim.y);
    //
    //    float shadowFactor = 0.0;
    //    int count = 0;
    //    int range = 1;
    //
    //    for (int x = -range; x <= range; x++) {
    //        for (int y = -range; y <= range; y++) {
    //            shadowFactor += CalcShadowFactorDir(shadowMapIndex, lightSpacePos, float2(dx * x, dy * y) /*, csm */);
    //            count++;
    //        }
    //    }
    //    return 1.0 - (shadowFactor / count);
    return 1.0f;
}

//float CalcShadowFactorPoint(int shadowMapIndex, vec3 lightToFragDir, float range) {
//    float shadow = 0.0;
//
//    float sampledDepth = texture(g_pointShadowMap[shadowMapIndex], lightToFragDir).r;
//    float distance = length(lightToFragDir);
//
//    if (sampledDepth + 0.02 <= distance) {
//        shadow = smoothstep(range, range * 0.75, distance);
//    }
//
//    return shadow;
//}
//
float CalcShadowFactorSpot(int shadowMapIndex, float4 lightSpacePos, float2 off) {
    float visibility = 1.0;

    float3 shadowCoords = lightSpacePos.xyz / lightSpacePos.w;
    if (shadowCoords.z > -1.0 && shadowCoords.z < 1.0) {
        float2 normCoords = 0.5 * shadowCoords.xy + 0.5;
        float2 uv = normCoords + off;

        // TODO: Depth sampler.
        float depth = g_texture2d[shadowMapIndex].Sample(g_sampler[0], uv).r;

        if (depth + EPSILON_D >= shadowCoords.z) {
            visibility = 0.0;
        }
    }

    return visibility;
}
//
//float CalcShadowFactorPCFSpot(int shadowMapIndex, vec4 lightSpacePos, float lightRange) {
//    ifloat2 texDim = textureSize(g_spotShadowMap[shadowMapIndex], 0);
//    float scale = 1.5;
//    float dx = scale * 1.0 / float(texDim.x);
//    float dy = scale * 1.0 / float(texDim.y);
//
//    float shadowFactor = 0.0;
//    int count = 0;
//    int range = 1;
//
//    for (int x = -range; x <= range; x++) {
//        for (int y = -range; y <= range; y++) {
//            shadowFactor += CalcShadowFactorSpot(shadowMapIndex, lightSpacePos, float2(dx * x, dy * y));
//            count++;
//        }
//    }
//
//    shadowFactor = 1.0 - (shadowFactor / count);
//    shadowFactor *= smoothstep(lightRange, 0, length(lightSpacePos));
//    return shadowFactor;
//}
//

#endif // !PASS_DEPTH