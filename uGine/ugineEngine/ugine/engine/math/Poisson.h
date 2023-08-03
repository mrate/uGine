#pragma once

#include "Math.h"

#include <vector>

namespace ugine::math {

void FastPoissonDisk2D(f32 maxX, f32 maxY, f32 minDistance, std::vector<glm::vec2>& output, u32 maxCount = 0);

}