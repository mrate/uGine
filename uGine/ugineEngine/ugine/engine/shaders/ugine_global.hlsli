#include "Shader_Types.h"

#define PI 3.14159265359
#define EPSILON 0.0001

// Compute a plane from 3 noncollinear points that form a triangle.
// This equation assumes a right-handed (counter-clockwise winding order)
// coordinate system to determine the direction of the plane normal.
Plane ComputePlane(float3 p0, float3 p1, float3 p2) {
    Plane plane;

    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;

    plane.N = normalize(cross(v0, v2));
    // Compute the distance to the origin using p0.
    plane.d = dot(plane.N, p0);

    return plane;
}

bool SphereInsidePlane(Sphere sphere, Plane plane) {
    return dot(plane.N, sphere.c) - plane.d < -sphere.r;
}

bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar) {
    bool result = true;

    if (sphere.c.z - sphere.r > zNear || sphere.c.z + sphere.r < zFar) {
        result = false;
    }

    for (int i = 0; i < 4 && result; i++) {
        if (SphereInsidePlane(sphere, frustum.planes[i])) {
            result = false;
        }
    }

    return result;
}

bool PointInsidePlane(float3 p, Plane plane) {
    return dot(plane.N, p) - plane.d < 0;
}

bool ConeInsidePlane(Cone cone, Plane plane) {
    float3 m = cross(cross(plane.N, cone.d), cone.d);
    float3 Q = cone.T + cone.d * cone.h - m * cone.r;

    return PointInsidePlane(cone.T, plane) && PointInsidePlane(Q, plane);
}

bool ConeInsideFrustum(Cone cone, Frustum frustum, float zNear, float zFar) {
    bool result = true;

    Plane nearPlane = { float3(0, 0, -1), -zNear };
    Plane farPlane = { float3(0, 0, 1), zFar };

    if (ConeInsidePlane(cone, nearPlane) || ConeInsidePlane(cone, farPlane)) {
        result = false;
    }

    for (int i = 0; i < 4 && result; i++) {
        if (ConeInsidePlane(cone, frustum.planes[i])) {
            result = false;
        }
    }

    return result;
}

float4 FullscreenQuad(uint vertexId, out float2 fragUV) {
    fragUV = float2((vertexId << 1) & 2, vertexId & 2);
    return float4(fragUV * 2.0f + -1.0f, 0.0f, 1.0f);
}

static const float2x2 BayerMatrix2 = {
    { 1.0 / 5.0, 3.0 / 5.0 },
    { 4.0 / 5.0, 2.0 / 5.0 },
};

static const float3x3 BayerMatrix3 = {
    { 3.0 / 10.0, 7.0 / 10.0, 4.0 / 10.0 },
    { 6.0 / 10.0, 1.0 / 10.0, 9.0 / 10.0 },
    { 2.0 / 10.0, 8.0 / 10.0, 5.0 / 10.0 },
};

static const float4x4 BayerMatrix4 = {
    { 1.0 / 17.0, 9.0 / 17.0, 3.0 / 17.0, 11.0 / 17.0 },
    { 13.0 / 17.0, 5.0 / 17.0, 15.0 / 17.0, 7.0 / 17.0 },
    { 4.0 / 17.0, 12.0 / 17.0, 2.0 / 17.0, 10.0 / 17.0 },
    { 16.0 / 17.0, 8.0 / 17.0, 14.0 / 17.0, 6.0 / 17.0 },
};

static const float BayerMatrix8[8][8] = {
    { 1.0 / 65.0, 49.0 / 65.0, 13.0 / 65.0, 61.0 / 65.0, 4.0 / 65.0, 52.0 / 65.0, 16.0 / 65.0, 64.0 / 65.0 },
    { 33.0 / 65.0, 17.0 / 65.0, 45.0 / 65.0, 29.0 / 65.0, 36.0 / 65.0, 20.0 / 65.0, 48.0 / 65.0, 32.0 / 65.0 },
    { 9.0 / 65.0, 57.0 / 65.0, 5.0 / 65.0, 53.0 / 65.0, 12.0 / 65.0, 60.0 / 65.0, 8.0 / 65.0, 56.0 / 65.0 },
    { 41.0 / 65.0, 25.0 / 65.0, 37.0 / 65.0, 21.0 / 65.0, 44.0 / 65.0, 28.0 / 65.0, 40.0 / 65.0, 24.0 / 65.0 },
    { 3.0 / 65.0, 51.0 / 65.0, 15.0 / 65.0, 63.0 / 65.0, 2.0 / 65.0, 50.0 / 65.0, 14.0 / 65.0, 62.0 / 65.0 },
    { 35.0 / 65.0, 19.0 / 65.0, 47.0 / 65.0, 31.0 / 65.0, 34.0 / 65.0, 18.0 / 65.0, 46.0 / 65.0, 30.0 / 65.0 },
    { 11.0 / 65.0, 59.0 / 65.0, 7.0 / 65.0, 55.0 / 65.0, 10.0 / 65.0, 58.0 / 65.0, 6.0 / 65.0, 54.0 / 65.0 },
    { 43.0 / 65.0, 27.0 / 65.0, 39.0 / 65.0, 23.0 / 65.0, 42.0 / 65.0, 26.0 / 65.0, 38.0 / 65.0, 22.0 / 65.0 },
};

float DitherMask2(in int2 pixel) {
    return BayerMatrix2[pixel.x % 2][pixel.y % 2];
}

float DitherMask3(in int2 pixel) {
    return BayerMatrix3[pixel.x % 3][pixel.y % 3];
}

float DitherMask4(in int2 pixel) {
    return BayerMatrix4[pixel.x % 4][pixel.y % 4];
}

float DitherMask8(in int2 pixel) {
    return BayerMatrix8[pixel.x % 8][pixel.y % 8];
}

float Dither(in int2 pixel) {
    return DitherMask8(pixel);
}

float3 temperature(float t) {
    const float3 c[10] = { float3(0.0 / 255.0, 2.0 / 255.0, 91.0 / 255.0), float3(0.0 / 255.0, 108.0 / 255.0, 251.0 / 255.0),
        float3(0.0 / 255.0, 221.0 / 255.0, 221.0 / 255.0), float3(51.0 / 255.0, 221.0 / 255.0, 0.0 / 255.0), float3(255.0 / 255.0, 252.0 / 255.0, 0.0 / 255.0),
        float3(255.0 / 255.0, 180.0 / 255.0, 0.0 / 255.0), float3(255.0 / 255.0, 104.0 / 255.0, 0.0 / 255.0), float3(226.0 / 255.0, 22.0 / 255.0, 0.0 / 255.0),
        float3(191.0 / 255.0, 0.0 / 255.0, 83.0 / 255.0), float3(145.0 / 255.0, 0.0 / 255.0, 65.0 / 255.0) };

    const float s = t * 10.0f;

    const int cur = int(s) <= 9 ? int(s) : 9;
    const int prv = cur >= 1 ? cur - 1 : 0;
    const int nxt = cur < 9 ? cur + 1 : 9;

    const float blur = 0.8f;

    const float wc = smoothstep(float(cur) - blur, float(cur) + blur, s) * (1.0f - smoothstep(float(cur + 1) - blur, float(cur + 1) + blur, s));
    const float wp = 1.0f - smoothstep(float(cur) - blur, float(cur) + blur, s);
    const float wn = smoothstep(float(cur + 1) - blur, float(cur + 1) + blur, s);

    const float3 r = wc * c[cur] + wp * c[prv] + wn * c[nxt];
    return float3(clamp(r.x, 0.0f, 1.0f), clamp(r.y, 0.0f, 1.0f), clamp(r.z, 0.0f, 1.0f));
}

//float4x4 Rotation(float3 axis, float angle) {
//    axis = normalize(axis);
//    float s = sin(angle);
//    float c = cos(angle);
//    float oc = 1.0 - c;
//
//    return mat4(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s, 0.0, oc * axis.x * axis.y + axis.z * s,
//        oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0, oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s,
//        oc * axis.z * axis.z + c, 0.0, 0.0, 0.0, 0.0, 1.0);
//}
//
//mat4 Translation(float3 pos) {
//    return mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, pos.x, pos.y, pos.z, 1);
//}
//
//bool FrustumCull(float4 frustumPlanes[6], AABB aabb) {
//    float3 points[8] = {
//        float3(aabb.mmin.x, aabb.mmin.y, aabb.mmin.z),
//        float3(aabb.mmax.x, aabb.mmin.y, aabb.mmin.z),
//        float3(aabb.mmin.x, aabb.mmax.y, aabb.mmin.z),
//        float3(aabb.mmax.x, aabb.mmax.y, aabb.mmin.z),
//        float3(aabb.mmin.x, aabb.mmin.y, aabb.mmax.z),
//        float3(aabb.mmax.x, aabb.mmin.y, aabb.mmax.z),
//        float3(aabb.mmin.x, aabb.mmax.y, aabb.mmax.z),
//        float3(aabb.mmax.x, aabb.mmax.y, aabb.mmax.z),
//    };
//
//    bool inside = false;
//    for (int i = 0; i < 6; ++i) {
//        float4 plane = frustumPlanes[i];
//
//        inside = false;
//        for (int j = 0; j < 8; ++j) {
//            float3 point = points[j];
//
//            if (plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w > 0.0f) {
//                inside = true;
//                break;
//            }
//        }
//
//        if (!inside) {
//            return false;
//        }
//    }
//    return true;
//}
//
//float3 AABBMin(float3 a, float3 b) {
//    return float3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
//}
//
//float3 AABBMax(float3 a, float3 b) {
//    return float3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
//}
//
//uint CalcLod(float distance) {
//    return distance < 5 ? 0 : distance < 10 ? 1 : distance < 20 ? 3 : distance < 40 ? 4 : 5;
//}
//
//bool IsSaturated(float3 v) {
//    return v == clamp(v, 0, 1);
//}
//
//bool IsSaturated(float2 v) {
//    return v == clamp(v, 0, 1);
//}
//
//float RaySphereIntersection(float3 origin, float3 direction, float3 center, float radius) {
//    float3 oc = origin - center;
//    float a = dot(direction, direction);
//    float b = 2.0 * dot(oc, direction);
//    float c = dot(oc, oc) - radius * radius;
//    float discriminant = b * b - 4 * a * c;
//    return discriminant < 0 ? -1.0 : (-b - sqrt(discriminant)) / (2.0 * a);
//}
//

float3 ViewSpaceFromDepth(float2 uv, Camera camera, float depth) {
    float2 screenSpace = uv * 2.0 - 1.0;
    float4 positionVS = mul(camera.invProj, float4(screenSpace, depth, 1.0));
    return positionVS.xyz / positionVS.w;
}

float3 WorldSpaceFromDepth(float2 uv, Camera camera, float depth) {
    float3 positionVS = ViewSpaceFromDepth(uv, camera, depth);
    return mul(camera.invView, float4(positionVS, 1)).xyz;
}

// Improved reconstruction of normals from depth buffer based on minimal depth discontinuities.
// https://wickedengine.net/2019/09/22/improved-normal-reconstruction-from-depth/
float3 NormalVSFromDepth(float2 uv, Camera camera, Texture2D depthTexture, SamplerState depthSampler) {
    float2 texelSize = 1.0f / camera.resolution;
    float2 offsets[] = {
        uv,                          // center
        uv - float2(texelSize.x, 0), // l
        uv + float2(texelSize.x, 0), // r
        uv - float2(0, texelSize.y), // t
        uv + float2(0, texelSize.y)  // b
    };

    float depths[] = {
        depthTexture.Sample(depthSampler, offsets[0]).r, // center
        depthTexture.Sample(depthSampler, offsets[1]).r, // l
        depthTexture.Sample(depthSampler, offsets[2]).r, // r
        depthTexture.Sample(depthSampler, offsets[3]).r, // t
        depthTexture.Sample(depthSampler, offsets[4]).r  // b
    };

    int h = abs(depths[1] - depths[0]) < abs(depths[2] - depths[0]) ? 1 : 2;
    int v = abs(depths[3] - depths[0]) < abs(depths[4] - depths[0]) ? 3 : 4;

    float3 P0 = ViewSpaceFromDepth(offsets[0], camera, depths[0]);
    float3 P1;
    float3 P2;

    if ((h == 2 && v == 4) || (h == 1 && v == 3)) {
        P1 = ViewSpaceFromDepth(offsets[h], camera, depths[h]);
        P2 = ViewSpaceFromDepth(offsets[v], camera, depths[v]);
    } else {
        P1 = ViewSpaceFromDepth(offsets[v], camera, depths[v]);
        P2 = ViewSpaceFromDepth(offsets[h], camera, depths[h]);
    }

    return normalize(cross(P2 - P0, P1 - P0));
}

float3 NormalVSFromDepthClassic(float2 uv, Camera camera, Texture2D<float> depthTexture, SamplerState depthSampler) {
    float2 uv0 = uv;                                    // center
    float2 uv1 = uv + float2(1, 0) / camera.resolution; // right
    float2 uv2 = uv + float2(0, 1) / camera.resolution; // top

    float3 P0 = ViewSpaceFromDepth(uv0, camera, depthTexture.Sample(depthSampler, uv0));
    float3 P1 = ViewSpaceFromDepth(uv1, camera, depthTexture.Sample(depthSampler, uv1));
    float3 P2 = ViewSpaceFromDepth(uv2, camera, depthTexture.Sample(depthSampler, uv2));

    return normalize(cross(P2 - P0, P1 - P0));
}
