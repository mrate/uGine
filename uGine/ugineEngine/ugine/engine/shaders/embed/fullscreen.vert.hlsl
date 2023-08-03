#include "fullscreen.hlsli"

#include "ugine_global.hlsli"

VSOutput main(out float4 pos : SV_Position, uint vertexId : SV_VertexID) {
    VSOutput output;

    pos = FullscreenQuad(vertexId, output.fragUV);

    return output;
}