#include "gizmo.hlsli"

float4 main()
    : SV_Target {
    return params.color;
}