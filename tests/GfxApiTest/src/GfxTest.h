#pragma once

#include <ugine/Color.h>
#include <gfxapi/CommandList.h>
#include <gfxapi/Device.h>
#include <gfxapi/Swapchain.h>

#include <ugine/engine/engine/Params.h>
#include <ugine/engine/system/Platform.h>

#include <ugineTools/Embed.h>

class GfxTest {
public:
    GfxTest(u32 width = 1280, u32 height = 1024, bool useImGui = false)
        : width_{ width }
        , height_{ height }
        , useImgui_{ false /*useImGui*/ } {
        using namespace ugine;
        using namespace ugine::gfxapi;

        EngineParams params{
            .appName = "GfxApiTest",
            .appVersion = { 1, 0, 0 },
            .fullscreen = false,
            .width = width,
            .height = height,
        };

        platform_ = Platform::Create(params);

        gfxapi::DeviceCreateInfo deviceCI{
            .vulkan = {
                .appName = "GfxApiTest",
                .appVer = {1, 0, 0}, 
                .engineName = "uGine", 
                .engineVer = {UGINE_VERSION_MAJOR, UGINE_VERSION_MINOR, UGINE_VERSION_FILE},
                .maxCommandBuffers = 3,
            },
            .validationLayers = true,
        };

        platform_->FillDeviceCreateInfo(deviceCI);
        device_ = Device::Create(deviceCI);
        swapchain_ = device_->GetSwapchain();

        if (useImgui_) {
            InitImGui();
        }
    }

    virtual ~GfxTest() {
        device_ = nullptr;

        if (useImgui_) {
            DestroyImGui();
        }
    }

    void Update() {
        using namespace ugine;

        for (;;) {
            auto event{ platform_->PollSystemEvent() };
            if (!event) {
                break;
            }

            if (event->type == SystemEvent::Type::Quit
                || (event->type == SystemEvent::Type::Input && event->input.type == InputEvent::Type::KeyDown
                    && event->input.keyDown.key == InputKeyCode::Key_Escape)) {
                quit_ = true;
            }
        }
        platform_->Update();

        if (useImgui_) {
            NewFrameImGui();
        }
    }

    void InitImGui() {}
    void DestroyImGui() {}
    void NewFrameImGui() {}
    void RenderImGui() {}

    virtual void Run() = 0;

    ugine::UniquePtr<ugine::Platform> platform_;
    ugine::UniquePtr<ugine::gfxapi::Device> device_;
    ugine::gfxapi::Swapchain* swapchain_{};
    u32 width_{};
    u32 height_{};
    bool quit_{};

    bool useImgui_{};
};
