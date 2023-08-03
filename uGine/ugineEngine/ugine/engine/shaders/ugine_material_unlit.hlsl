struct MaterialUnlitFragment {
    float3 emissive;
#ifdef MATERIAL_MASKED
    float opacityMask;
#endif
#ifdef MATERIAL_OPACITY
    float opacity;
#endif
};
