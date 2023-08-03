#include "Shader_Common.h"

// TODO: Problem with wrong hlsl alignment...
struct Vertex {
    float position0;
    float position1;
    float position2;
    float normal0;
    float normal1;
    float normal2;
    float tangent0;
    float tangent1;
    float tangent2;
    float uv0;
    float uv1;
};

struct SkinVertex {
    float4 inJointIndices;
    float4 inJointWeights;
};

BINDING(0, 0) StructuredBuffer<Vertex> inVertex;
BINDING(0, 1) StructuredBuffer<SkinVertex> inSkin;
BINDING(0, 2) StructuredBuffer<float4x4> inMatrix;
BINDING(0, 3) RWStructuredBuffer<Vertex> outVertex;

PUSH_CONSTANT struct Params {
    uint vertexCount;
} params;

float4x4 inverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

[numthreads(256, 1, 1)] void main(uint3 dispIndex
                                  : SV_DispatchThreadID) {
    uint index = dispIndex.x;

    if (index < params.vertexCount) {
        SkinVertex v = inSkin[index];

        float4 weight = v.inJointWeights;
        float4 idx = v.inJointIndices;

        float4x4 skinMat
            = weight.x * inMatrix[uint(idx.x)] + weight.y * inMatrix[uint(idx.y)] + weight.z * inMatrix[uint(idx.z)] + weight.w * inMatrix[uint(idx.w)];

        Vertex input = inVertex[index];
        float3 position = mul(skinMat, float4(input.position0, input.position1, input.position2, 1.0)).xyz;

        Vertex oV;
        oV.position0 = position.x;
        oV.position1 = position.y;
        oV.position2 = position.z;
        oV.uv0 = input.uv0;
        oV.uv1 = input.uv1;

        float4x4 skinNormal = transpose(inverse(skinMat));
        float3 normal = normalize((mul(skinNormal, float4(input.normal0, input.normal1, input.normal2, 1.0)).xyz));
        float3 tangent = normalize((mul(skinNormal, float4(input.tangent0, input.tangent1, input.tangent2, 1.0)).xyz));

        oV.normal0 = normal.x;
        oV.normal1 = normal.y;
        oV.normal2 = normal.z;
        oV.tangent0 = tangent.x;
        oV.tangent1 = tangent.y;
        oV.tangent2 = tangent.z;

        outVertex[index] = oV;
    }
}
