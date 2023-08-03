#include "Shader_Common.h"

#include "Bindings_Material.hlsl"

#include "ugine_material_unlit.hlsl"
#include "ugine_material_vertex.hlsl"

struct VSOutput {
    LOCATION(0) float2 fragUV : TEXCOORD0;
};

VSOutput main(VSInput input, out float4 outPosition : SV_Position) {
    float4x4 model = g_draw.model;
    float4x4 normal = g_draw.normal;

#if defined(MATERIAL_INSTANCE)
    float4x4 instance = float4x4(                                   //
        input.instance0.x, input.instance1.x, input.instance2.x, 0, //
        input.instance0.y, input.instance1.y, input.instance2.y, 0, //
        input.instance0.z, input.instance1.z, input.instance2.z, 0, //
        input.instance0.w, input.instance1.w, input.instance2.w, 1  //
    );
    model = model * instance;
#endif // MATERIAL_INSTANCE

    float4 position = float4(input.position + material_vertex_offset(), 1.0);

    float4 modelPos = mul(model, position);
    float4 projViewModel = mul(g_camera.viewProj, modelPos);

    outPosition = projViewModel;

    VSOutput output;
    output.fragUV = input.texcoord;
    return output;
}
