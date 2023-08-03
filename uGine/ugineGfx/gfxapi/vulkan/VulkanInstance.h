#pragma once

#include "Vulkan.h"

#include <ugine/Error.h>
#include <ugine/Span.h>
#include <ugine/Vector.h>

#include <optional>

namespace ugine::gfxapi {

struct DeviceCreateInfo;
class VulkanDevice;

class VulkanInstance {
public:
    VulkanInstance(const DeviceCreateInfo& info);
    ~VulkanInstance();

    vk::Instance VkInstance() { return instance_.get(); }

    u32 VkVersion() const { return version_; }

    std::optional<DeviceDescriptor> FindFirstSuitableDevice(vk::SurfaceKHR surface, Span<const char*> deviceExtensions);

    bool SupportsDebug() const { return supportsDebug_; }

private:
    bool CheckDebugSupport(Vector<const char*>& extensions);
    bool CheckValidationLayerSupport() const;
    void SetupDebugMessenger();

    vk::UniqueInstance instance_;
    VkDebugUtilsMessengerEXT debugMessenger_{};

    bool enableValidationLayers_{};
    bool supportsDebug_{};

    u32 version_{ VK_API_VERSION_1_3 };
};

} // namespace ugine::gfxapi
