#ifndef _SHADER_LIGHTCULL_FRUSTUMS_H_
#define _SHADER_LIGHTCULL_FRUSTUMS_H_

#include "Shader_Common.h"

#define LIGHT_CULL_BLOCK_SIZE 16
#define LIGHT_LIST_SIZE 1024

UGINE_NAMESPACE_BEGIN

PUSH_CONSTANT struct STRUCT_ALIGN LightCullFrustumParams {
    float4x4 inverseProjection; // TODO:
    uint2 screenResolution;
    uint2 dispatchResolution;
} PUSH_CONSTANT_NAME(params);

UGINE_NAMESPACE_END

#endif // _SHADER_LIGHTCULL_FRUSTUMS_H_