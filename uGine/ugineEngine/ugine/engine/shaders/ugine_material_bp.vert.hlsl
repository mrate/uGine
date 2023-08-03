#include "Shader_Common.h"

#include "Bindings_Material.hlsl"

#include "ugine_material_bp.hlsl"
#include "ugine_material_vertex.hlsl"

struct VSOutput {
    LOCATION(0) float2 fragUV : TEXCOORD0;
    LOCATION(1) float3 positionVS : POSITION0;

#if !defined(PASS_DEPTH)
    LOCATION(2) float3 tangentWS : TANGENT0;
    LOCATION(3) float3 bitangentWS : BINORMAL0;
    LOCATION(4) float3 normalWS : NORMAL0;
    LOCATION(5) float3 positionWS : POSITION1;
    LOCATION(6) float3 positionM : POSITION2;
#endif
};

VSOutput main(VSInput input, out float4 outPosition : SV_Position) {
    VSOutput output;

#if defined(MATERIAL_INSTANCE)
    float4x4 instance = float4x4(                                                   //
        input.instance0.x, input.instance0.y, input.instance0.z, input.instance0.w, //
        input.instance1.x, input.instance1.y, input.instance1.z, input.instance1.w, //
        input.instance2.x, input.instance2.y, input.instance2.z, input.instance2.w, //
        0, 0, 0, 1                                                                  //
    );
    float4x4 model = mul(g_draw.model, instance);
    float4x4 normal = mul(g_draw.normal, instance);
#else // MATERIAL_INSTANCE
    float4x4 model = g_draw.model;
    float4x4 normal = g_draw.normal;
#endif // MATERIAL_INSTANCE

    float4 position = float4(input.position + material_vertex_offset(), 1.0);
    float4 modelPos = mul(model, position);
    
    outPosition = mul(g_camera.viewProj, modelPos);

    output.fragUV = input.texcoord;

    float4 viewModel = mul(g_camera.view, modelPos);
    output.positionVS = viewModel.xyz;

#if !defined(PASS_DEPTH)
    output.positionWS = modelPos.xyz;
    output.positionM = position.xyz;

    output.tangentWS = mul(model, float4(input.tangent, 0.0)).xyz;
    output.bitangentWS = mul(model, float4(cross(input.normal, input.tangent), 0.0)).xyz;
    output.normalWS = mul(normal, float4(input.normal, 0.0)).xyz;
#endif

    return output;
}
