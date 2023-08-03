#include "Shader_Material.h"
#include "Shader_Types.h"

UNIFORM_BUFFER(DATASET_GLOBAL, SLOT_GLOBAL, Global, g_global);
UNIFORM_BUFFER(DATASET_GLOBAL, SLOT_CAMERA, Camera, g_camera);

#if !defined(PASS_DEPTH)

BINDING(DATASET_GLOBAL, SLOT_LIGHTS) StructuredBuffer<Light> g_lights;
BINDING(DATASET_GLOBAL, SLOT_SHADOWS) StructuredBuffer<float4x4> g_shadows;

BINDING(DATASET_GLOBAL, SLOT_LIGHT_GRID) Texture2D<uint2> g_lightGrid;
BINDING(DATASET_GLOBAL, SLOT_LIGHT_INDEX) StructuredBuffer<uint> g_lightIndex;

#endif

BINDING(DATASET_BINDLESS_TEXTURES, 0) Texture2D g_texture2d[];
BINDING(DATASET_BINDLESS_TEXTURES, 0) Texture2DArray g_texture2dArray[];
BINDING(DATASET_BINDLESS_TEXTURES, 0) TextureCube g_textureCube[];

BINDING(DATASET_BINDLESS_SAMPLERS, 0) SamplerState g_sampler[];

UNIFORM_BUFFER(DATASET_DRAW, SLOT_DRAW, Draw, g_draw);
