struct MaterialBpFragment {
    float3 albedo;
    float3 emissive;
    float3 specular;
    float specularPower;
    float3 normal;
    bool hasNormal;

#ifdef MATERIAL_HEIGHT_MAP
    float3 height;
#endif

#ifdef MATERIAL_MASKED
    float opacityMask;
#endif

#ifdef MATERIAL_OPACITY
    float opacity;
#endif
};
