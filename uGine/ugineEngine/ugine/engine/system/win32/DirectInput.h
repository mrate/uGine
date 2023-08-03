#pragma once

#include <ugine/Memory.h>
#include <ugine/Vector.h>
#include <ugine/engine/input/InputController.h>

#ifdef _WIN32
#include "XControllerDevice.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <wrl/client.h>

#include <map>
#include <memory>
#include <string>

namespace ugine::win32 {

class DirectInputDevice;

class DirectInput : public std::enable_shared_from_this<DirectInput> {
public:
    DirectInput();
    ~DirectInput();

    Vector<XControllerDevice::Info> AttachedControllers(IAllocator& allocator) const;
    UniquePtr<InputController> CreateController(IAllocator& allocator, std::string_view id);

private:
    struct Controller {
        std::wstring name;
        Microsoft::WRL::ComPtr<IDirectInputDevice8> handle;
    };

    static BOOL CALLBACK StaticEnumerateGameControllers(LPCDIDEVICEINSTANCE devInst, LPVOID pvRef);

    bool EnumerateGameControllers(LPCDIDEVICEINSTANCE devInst);

    Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
    std::map<std::string, Controller> deviceInstances_;
};
#endif

} // namespace ugine::input
