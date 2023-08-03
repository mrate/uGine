#ifndef _UGINE_SURFACE_H_
#define _UGINE_SURFACE_H_

struct Surface {
    float3 wsP;
    float3 wsN;
    float3 wsV;

    float3 albedo;
    float3 emissiveColor;
    float specularPower;
};

#endif // _UGINE_SURFACE_H_