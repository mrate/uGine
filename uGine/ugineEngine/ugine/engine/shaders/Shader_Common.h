#ifndef _SHADER_COMMON_H_
#define _SHADER_COMMON_H_

#ifdef __cplusplus

#include <glm/glm.hpp>
#include <stdint.h>

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using float4x4 = glm::mat4;
using float3x3 = glm::mat3;

using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

using uint = u32;
using sint = i32;

namespace ugine::shaders {

#define STRUCT_ALIGN alignas(16)
#define STORAGE_SIZE 1

#define UGINE_NAMESPACE_BEGIN namespace ugine::shaders {
#define UGINE_NAMESPACE_END }

#define UNIFORM_BUFFER(DATASET, BINDING, TYPE, NAME)
#define PUSH_CONSTANT_NAME(NAME)

#define BEGIN_STORAGE(DATASET, BINDING, NAME)
#define END_STORAGE(VARIABLE)

#define UNIFORM_TEXTURE_2D(...)
#define UNIFORM_UTEXTURE_2D(...)
#define UNIFORM_TEXTURE_2D_ARRAY(...)
#define UNIFORM_IMAGE_2D(...)

#define IMAGE_SAMPLER
#define BINDING(...)
#define LOCATION(...)
#define PUSH_CONSTANT

} // namespace ugine::shaders

#else // __cplusplus

#define UGINE_NAMESPACE_BEGIN
#define UGINE_NAMESPACE_END

#define STRUCT_ALIGN
#define STORAGE_SIZE

#define UNIFORM_BUFFER(DATASET, SLOT, TYPE, NAME)                                                                                                              \
    BINDING(DATASET, SLOT) cbuffer TYPE##CB { TYPE NAME; }

#define PUSH_CONSTANT_NAME(NAME) NAME

#define RW_STORAGE_BUFFER(DATASET, SLOT, NAME) BINDING(DATASET, SLOT) RWStructuredBuffer<TYPE> NAME
#define STORAGE_BUFFER(DATASET, SLOT, NAME) BINDING(DATASET, SLOT) StructuredBuffer<TYPE> NAME

#define UNIFORM_TEXTURE_2D(DATASET, SLOT, NAME) BINDING(DATASET, SLOT) Texture2D NAME
#define UNIFORM_TEXTURE_2D_ARRAY(DATASET, SLOT, NAME, COUNT) BINDING(DATASET, SLOT) Texture2D NAME[COUNT]
#define UNIFORM_UTEXTURE_2D(DATASET, SLOT, NAME) BINDING(DATASET, SLOT) uniform usampler2D NAME
#define UNIFORM_UIMAGE_2D(DATASET, SLOT, TYPE, NAME) BINDING(DATASET, SLOT) uniform readonly uimage2D NAME

#define IMAGE_SAMPLER [[vk::combinedImageSampler]]
#define BINDING(DATASET, SLOT) [[vk::binding(SLOT, DATASET)]]
#define LOCATION(N) [[vk::location(N)]]
#define PUSH_CONSTANT [[vk::push_constant]]

#endif // __cplusplus

#endif // _SHADER_COMMON_H_