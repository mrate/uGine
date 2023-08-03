#include <gtest/gtest.h>

#include <cstdlib>
#include <ctime>
#include <iostream>

//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
//#define GLM_FORCE_SIMD_AVX2
//#define GLM_FORCE_INTRINSICS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define NUM_VERT 1000000000
#define PI 3.14159f

TEST(Glm, Benchmark) {
    //float* buf = new float[NUM_VERT + 4];

    //std::srand((unsigned int)(std::time(nullptr)));
    //for (size_t i = 0; i < NUM_VERT + 4; i++)
    //    buf[i] = float(std::rand() % 100) * PI;

    //glm::mat4 Model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f));
    //Model[3][2] = 0.11f;
    //Model[2][0] = -0.1f;
    //Model[0][1] = 0.04f;
    //Model[1][3] = -0.2f;

    //double sum = 0.0;
    //glm::vec4 Pos, Trans;

    //using Clock = std::chrono::high_resolution_clock;
    //const auto start{ Clock::now() };

    //for (size_t i = 0; i < NUM_VERT; i++) {
    //    Pos.x = buf[i];
    //    Pos.y = buf[i + 1];
    //    Pos.z = buf[i + 2];
    //    Pos.w = buf[i + 3];
    //    Trans = Model * Pos;
    //    sum += glm::length(Trans);
    //}

    //const auto sec{ std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count() / 1000.0f };

    //std::cout << "total time = " << sec << std::endl;
    //std::cout << "  sum = " << sum << std::endl;

    //delete[] buf;
}