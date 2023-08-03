#include "Shader_Common.h"

// TODO:
#define POSTPROCESS_THREAD_COUNT 16

BINDING(0, 0) Texture2D<float> inputImage;
BINDING(0, 1) RWTexture2D<float> outputImage;

[numthreads(POSTPROCESS_THREAD_COUNT, POSTPROCESS_THREAD_COUNT, 1)] void main(in uint3 id
                                                                              : SV_DispatchThreadID) {
    uint width, height;
    inputImage.GetDimensions(width, height);

    uint2 screenCoordinates = uint2(id.xy);
    float color = 0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            color += inputImage[screenCoordinates + int2(x, y)];
        }
    }
    float finalColor = color / 16.0;
    outputImage[screenCoordinates] = finalColor;
}
