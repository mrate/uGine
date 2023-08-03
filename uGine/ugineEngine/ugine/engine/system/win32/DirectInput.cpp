#ifdef _WIN32

#include "DirectInput.h"
#include "DirectInputDevice.h"

#include <ugine/Error.h>
#include <ugine/String.h>
#include <ugine/StringUtils.h>

#include <dinput.h>

namespace {
bool GuidToString(const GUID& guid, std::string& result) {
    LPOLESTR str;
    auto res = StringFromCLSID(guid, &str);
    result = ugine::ToString(str);
    CoTaskMemFree(str);

    return SUCCEEDED(res);
}

bool GuidFromString(std::string_view guidStr, GUID& result) {
    std::wstring wstr = ugine::ToWString(guidStr.data());

    return SUCCEEDED(CLSIDFromString(wstr.c_str(), &result));
}
} // namespace

namespace ugine::win32 {

DirectInput::DirectInput() {
    HINSTANCE instance = GetModuleHandle(nullptr);

    if (FAILED(DirectInput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, NULL))) {
        UGINE_THROW(Error, "Failed to create the main DirectInput 8 COM object!");
    }

    if (FAILED(directInput_->EnumDevices(DI8DEVCLASS_GAMECTRL, &StaticEnumerateGameControllers, this, DIEDFL_ATTACHEDONLY))) {
        UGINE_THROW(Error, "Failed to enumerate DirectInput devices!");
    }
}

DirectInput::~DirectInput() {
    for (auto& device : deviceInstances_) {
        device.second.handle->Unacquire();
    }
}

BOOL CALLBACK DirectInput::StaticEnumerateGameControllers(LPCDIDEVICEINSTANCE devInst, LPVOID pvRef) {
    auto instance = static_cast<DirectInput*>(pvRef);
    return instance->EnumerateGameControllers(devInst);
}

bool DirectInput::EnumerateGameControllers(LPCDIDEVICEINSTANCE devInst) {
    Microsoft::WRL::ComPtr<IDirectInputDevice8> device;

    if (SUCCEEDED(directInput_->CreateDevice(devInst->guidInstance, &device, NULL))) {
        std::string guid;
        GuidToString(devInst->guidInstance, guid);

        deviceInstances_.insert(std::make_pair(guid, Controller{ devInst->tszInstanceName, device }));
    }

    return DIENUM_CONTINUE;
}

Vector<XControllerDevice::Info> DirectInput::AttachedControllers(IAllocator& allocator) const {
    Vector<XControllerDevice::Info> result{ allocator };

    for (auto device : deviceInstances_) {
        result.PushBack({ device.second.name, device.first });
    }

    return result;
}

UniquePtr<InputController> DirectInput::CreateController(IAllocator& allocator, std::string_view id) {
    Microsoft::WRL::ComPtr<IDirectInputDevice8> device;

    // When ID is empty use first controller. Otherwise use only controller with given ID.
    GUID guid;
    bool validGuid = GuidFromString(id, guid);
    bool continueOnError = id.empty() || !validGuid;

    UniquePtr<DirectInputDevice> instance;
    for (auto controller : deviceInstances_) {
        if (id.empty() || !validGuid || controller.first == id) {
            instance = MakeUnique<DirectInputDevice>(allocator, controller.second.handle, shared_from_this());

            if (instance || !continueOnError) {
                break;
            }
        }
    }

    return instance;
}

} // namespace ugine::win32

#endif
