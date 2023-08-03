#pragma once

namespace ugine::gfxapi {

class WaitableEvent {
public:
    virtual ~WaitableEvent() {}

    virtual bool Wait(int timeout = 0) const = 0;
};

} // namespace ugine::gfxapi
