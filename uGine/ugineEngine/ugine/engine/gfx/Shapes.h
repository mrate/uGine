#pragma once

#include "Material.h"

namespace ugine {
std::pair<std::vector<MaterialVertex>, std::vector<u16>> CubeVertices(f32 size);
std::pair<std::vector<MaterialVertex>, std::vector<u16>> SphereVertices(f32 diameter, f32 sectorCount, f32 stackCount);
std::pair<std::vector<MaterialVertex>, std::vector<u16>> PlaneVertices(f32 width, f32 height);
} // namespace ugine