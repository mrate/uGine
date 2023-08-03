#ifdef _WIN32
#include "DirectInputDevice.h"

#include <ugine/Error.h>
#include <ugine/Log.h>

// TODO:
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace ugine::win32 {

DirectInputDevice::DirectInputDevice(const Microsoft::WRL::ComPtr<IDirectInputDevice8>& controller, const std::shared_ptr<DirectInput>& di)
    : controller_(controller)
    , directInput_(di) {
    if (!TryInitializeController(controller_.Get())) {
        UGINE_THROW(Error, "Failed to initialize DirectInput controller.");
    }
}

DirectInputDevice::~DirectInputDevice() {
    if (controller_) {
        controller_->Unacquire();
        controller_ = nullptr;
    }
}

HRESULT DirectInputDevice::Poll() {
    HRESULT hr;

    ZeroMemory(&currentState_, sizeof(DIJOYSTATE));

    hr = controller_->Poll();
    if (FAILED(hr)) {
        // Reacquire controller.
        hr = controller_->Acquire();
        while (hr == DIERR_INPUTLOST) {
            hr = controller_->Acquire();
        }

        if ((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED)) {
            return E_FAIL;
        }

        if (hr == DIERR_OTHERAPPHASPRIO) {
            return S_OK;
        }
    }

    return controller_->GetDeviceState(sizeof(DIJOYSTATE), &currentState_);
}

void DirectInputDevice::ReadState() {
    if (controller_ == nullptr || FAILED(Poll())) {
        return;
    }

    Analog left{ static_cast<f32>(currentState_.lX), static_cast<f32>(-currentState_.lY) };
    Analog right{ static_cast<f32>(currentState_.lRz), static_cast<f32>(-currentState_.lZ) };

    Buttons buttons{
        (currentState_.rgbButtons[0] & 0x80) != 0,
        (currentState_.rgbButtons[1] & 0x80) != 0,
        (currentState_.rgbButtons[2] & 0x80) != 0,
        (currentState_.rgbButtons[3] & 0x80) != 0,
    };

    left.x /= ANALOG_RANGE;
    left.y /= ANALOG_RANGE;
    left.pressed = currentState_.lZ != 0;

    right.x /= ANALOG_RANGE;
    right.y /= ANALOG_RANGE;
    right.pressed = currentState_.lRz != 0;

    SetState(left, right, buttons);
}

bool DirectInputDevice::IsConnected() const {
    return controller_ != nullptr;
}

bool DirectInputDevice::TryInitializeController(LPDIRECTINPUTDEVICE8 controller) const {
    // TODO: hwnd and error checking.
    controller->SetCooperativeLevel(0, DISCL_BACKGROUND | DISCL_EXCLUSIVE);

    if (FAILED(controller->SetDataFormat(&c_dfDIJoystick))) {
        return false;
    }

    if (FAILED(controller->Acquire())) {
        return false;
    }

    if (FAILED(controller->EnumObjects(&StaticSetGameControllerAxesRanges, controller, DIDFT_AXIS))) {
        return false;
    }

    return true;
}

BOOL CALLBACK DirectInputDevice::StaticSetGameControllerAxesRanges(LPCDIDEVICEOBJECTINSTANCE devObjInst, LPVOID pvRef) {
    LPDIRECTINPUTDEVICE8 gameController = reinterpret_cast<LPDIRECTINPUTDEVICE8>(pvRef);
    gameController->Unacquire();

    DIPROPRANGE gameControllerRange;
    gameControllerRange.lMin = -ANALOG_RANGE;
    gameControllerRange.lMax = ANALOG_RANGE;

    gameControllerRange.diph.dwSize = sizeof(DIPROPRANGE);
    gameControllerRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);

    gameControllerRange.diph.dwHow = DIPH_BYID;
    gameControllerRange.diph.dwObj = devObjInst->dwType;

    if (FAILED(gameController->SetProperty(DIPROP_RANGE, &gameControllerRange.diph))) {
        return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}

} // namespace ugine::win32

#endif
