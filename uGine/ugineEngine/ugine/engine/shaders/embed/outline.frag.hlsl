#include "fullscreen.hlsli"

// TODO: Bindless.
BINDING(0, 0) Texture2D<uint> stencilTexture;

PUSH_CONSTANT struct Params {
    float3 color;
    uint mask;
} params;

#define KERNEL_SIZE 1

float4 main(VSOutput input)
    : SV_TARGET {
    int2 size;
    stencilTexture.GetDimensions(size.x, size.y);
    int2 posStencil = int2(size * input.fragUV);

    float acc = 0;
    float samples = 0;
    for (int x = -KERNEL_SIZE; x <= KERNEL_SIZE; ++x) {
        for (int y = -KERNEL_SIZE; y <= KERNEL_SIZE; ++y) {
            samples += 1;
            int2 coord = posStencil + int2(x, y);

            uint value = 0;
            if (coord.x >= 0 && coord.x < size.x && coord.y >= 0 && coord.y < size.y) {
                value = stencilTexture[coord].r;
            }
            acc += (value & params.mask) > 0 ? 1 : 0;
        }
    }

    if (acc == 0 || acc >= samples) {
        discard;
    }

    return float4(params.color, 1);
}