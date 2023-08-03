#pragma once

#include <ugine/Ugine.h>

namespace ugine {

class Camera;
class Engine;

struct Transformation;

class CameraController {
public:
    void Init(const Transformation& transformation);

    void Update(Engine& engine, Transformation& transformation);

private:
    f32 yaw_{};
    f32 pitch_{};
};

} // namespace ugine
