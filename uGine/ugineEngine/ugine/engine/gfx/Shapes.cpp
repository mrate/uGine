#include "Shapes.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace ugine {

std::pair<std::vector<MaterialVertex>, std::vector<u16>> CubeVertices(f32 size) {
    const auto halfSize{ size * 0.5f };

    const std::vector<MaterialVertex> vertices = { {
        // Front
        { { -halfSize, -halfSize, halfSize }, { 0, 0, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { halfSize, -halfSize, halfSize }, { 0, 0, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { halfSize, halfSize, halfSize }, { 0, 0, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -halfSize, halfSize, halfSize }, { 0, 0, 1.0f }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },

        // Back
        { { halfSize, -halfSize, -halfSize }, { 0, 0, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -halfSize, -halfSize, -halfSize }, { 0, 0, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { -halfSize, halfSize, -halfSize }, { 0, 0, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { halfSize, halfSize, -halfSize }, { 0, 0, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },

        // Right
        { { halfSize, -halfSize, halfSize }, { 1.0f, 0, 0 }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
        { { halfSize, -halfSize, -halfSize }, { 1.0f, 0, 0 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
        { { halfSize, halfSize, -halfSize }, { 1.0f, 0, 0 }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
        { { halfSize, halfSize, halfSize }, { 1.0f, 0, 0 }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },

        // Left`
        { { -halfSize, -halfSize, -halfSize }, { -1.0f, 0, 0 }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
        { { -halfSize, -halfSize, halfSize }, { -1.0f, 0, 0 }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
        { { -halfSize, halfSize, halfSize }, { -1.0f, 0, 0 }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
        { { -halfSize, halfSize, -halfSize }, { -1.0f, 0, 0 }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },

        // Bottom
        { { -halfSize, -halfSize, -halfSize }, { 0, -1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { halfSize, -halfSize, -halfSize }, { 0, -1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { halfSize, -halfSize, halfSize }, { 0, -1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -halfSize, -halfSize, halfSize }, { 0, -1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },

        // Top
        { { -halfSize, halfSize, halfSize }, { 0, 1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { halfSize, halfSize, halfSize }, { 0, 1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { halfSize, halfSize, -halfSize }, { 0, 1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { -halfSize, halfSize, -halfSize }, { 0, 1.0f, 0 }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
    } };

    const std::vector<u16> indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Right
        12, 13, 14, 14, 15, 12, // Left
        16, 17, 18, 18, 19, 16, // Bottom
        20, 21, 22, 22, 23, 20, // Top
    };

    return std::make_pair(vertices, indices);
}

std::pair<std::vector<MaterialVertex>, std::vector<u16>> PlaneVertices(f32 width, f32 height) {
    const auto halfWidth{ width * 0.5f };
    const auto halfHeight{ height * 0.5f };

    const std::vector<MaterialVertex> vertices = { {
        // Front
        { { -halfWidth, -halfHeight, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
        { { halfWidth, -halfHeight, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
        { { -halfWidth, halfHeight, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
        { { halfWidth, halfHeight, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
    } };

    const std::vector<u16> indices = { 0, 1, 2, 1, 3, 2 };

    return std::make_pair(vertices, indices);
}

std::pair<std::vector<MaterialVertex>, std::vector<u16>> SphereVertices(f32 diameter, f32 sectorCount, f32 stackCount) {
    // http://www.songho.ca/opengl/gl_sphere.html

    f32 radius{ diameter * 0.5f };

    f32 x, y, z, xy;                           // vertex position
    f32 nx, ny, nz, lengthInv = 1.0f / radius; // vertex normal
    f32 s, t;                                  // vertex texCoord

    f32 sectorStep = 2 * glm::pi<f32>() / sectorCount;
    f32 stackStep = glm::pi<f32>() / stackCount;
    f32 sectorAngle, stackAngle;

    std::vector<MaterialVertex> vertices;
    for (int i{}; i <= stackCount; ++i) {
        stackAngle = glm::pi<f32>() / 2 - i * stackStep; // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);                  // r * cos(u)
        z = radius * sinf(stackAngle);                   // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for (int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep; // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;

            // vertex tex coord (s, t) range between [0, 1]
            s = (f32)j / sectorCount;
            t = (f32)i / stackCount;

            // TODO: Tangent.
            vertices.push_back({ { x, y, z }, { nx, ny, nz }, { 0, 0, 0 }, { s, t } });
        }
    }

    // generate CCW index list of sphere triangles
    std::vector<u16> indices;
    int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
        k1 = static_cast<int>(i * (sectorCount + 1)); // beginning of current stack
        k2 = static_cast<int>(k1 + sectorCount + 1);  // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    return std::make_pair(vertices, indices);
}

} // namespace ugine