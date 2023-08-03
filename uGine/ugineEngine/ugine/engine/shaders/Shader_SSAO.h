#ifndef _SHADER_SSAO_H_
#define _SHADER_SSAO_H_

#include "Shader_Common.h"

#define UGINE_SSAO_KERNEL_SIZE (32)

UGINE_NAMESPACE_BEGIN

struct STRUCT_ALIGN Ssao {
    float radius;
    float bias;
    float strength;
    float4 samples[UGINE_SSAO_KERNEL_SIZE];
};

UGINE_NAMESPACE_END

#endif // _SHADER_SSAO_H_