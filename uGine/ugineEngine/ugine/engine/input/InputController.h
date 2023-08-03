#pragma once

#include <ugine/Ugine.h>

namespace ugine {

class InputController {
public:
    virtual ~InputController() {}

    virtual void ReadState() = 0;
    virtual bool IsConnected() const = 0;

    virtual f32 LeftAnalogX() const = 0;
    virtual f32 LeftAnalogY() const = 0;
    virtual bool LeftAnalogPressed() const = 0;
    virtual f32 RightAnalogX() const = 0;
    virtual f32 RightAnalogY() const = 0;
    virtual bool RightAnalogPressed() const = 0;

    virtual f32 ButtonA() const = 0;
    virtual f32 ButtonB() const = 0;
    virtual f32 ButtonX() const = 0;
    virtual f32 ButtonY() const = 0;
};

} // namespace ugine
