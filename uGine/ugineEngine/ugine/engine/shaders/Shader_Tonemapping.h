#ifndef _SHADER_TONEMAPPING_H_
#define _SHADER_TONEMAPPING_H_

#include "Shader_Types.h"

UGINE_NAMESPACE_BEGIN

struct STRUCT_ALIGN Tonemapping {
    float gamma;
    float exposure;
    int algorithm;
};

UGINE_NAMESPACE_END

#endif // _SHADER_TONEMAPPING_H_