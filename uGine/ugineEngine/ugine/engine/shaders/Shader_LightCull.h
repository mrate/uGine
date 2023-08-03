#ifndef _SHADER_LIGHTCULL_H_
#define _SHADER_LIGHTCULL_H_

#include "Shader_Common.h"

#define LIGHT_CULL_BLOCK_SIZE 16
#define LIGHT_LIST_SIZE 1024

UGINE_NAMESPACE_BEGIN

struct STRUCT_ALIGN LightCullParams {
    float4x4 inverseProjection; // TODO:
    float4x4 view;
    uint3 numThreadGroups;
    uint lightsCount;
};

UGINE_NAMESPACE_END

#endif // _SHADER_LIGHTCULL_H_