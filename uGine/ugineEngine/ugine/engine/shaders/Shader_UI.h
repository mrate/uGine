#ifndef _SHADER_UI_H_
#define _SHADER_UI_H_

#include "Shader_Common.h"

UGINE_NAMESPACE_BEGIN

PUSH_CONSTANT struct STRUCT_ALIGN UiParams {
    float2 scale;
    float2 translate;
    uint2 textureSampler;
    uint flags;
} PUSH_CONSTANT_NAME(params);

UGINE_NAMESPACE_END

#endif // _SHADER_UI_H_