#pragma once

#ifdef _WIN32

#include "XControllerDevice.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <wrl/client.h>

#include <memory>
#include <vector>

namespace ugine::win32 {

class DirectInput;

class DirectInputDevice final : public XControllerDevice {
public:
    DirectInputDevice(const Microsoft::WRL::ComPtr<IDirectInputDevice8>& controller, const std::shared_ptr<DirectInput>& di);
    ~DirectInputDevice();

    void ReadState() override;
    bool IsConnected() const override;

private:
    inline static const long ANALOG_RANGE{ 100 };

    static BOOL CALLBACK StaticSetGameControllerAxesRanges(LPCDIDEVICEOBJECTINSTANCE devObjInst, LPVOID pvRef);

    bool TryInitializeController(LPDIRECTINPUTDEVICE8 controller) const;
    HRESULT Poll();

    Microsoft::WRL::ComPtr<IDirectInputDevice8> controller_;
    DIJOYSTATE currentState_;

    std::shared_ptr<DirectInput> directInput_;
};

} // namespace ugine::win32

#endif
