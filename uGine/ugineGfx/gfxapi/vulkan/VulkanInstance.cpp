#include "VulkanInstance.h"

#include <gfxapi/Device.h>
#include <gfxapi/Error.h>

#include <ugine/Log.h>

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

using namespace ugine::gfxapi;

namespace {

bool CheckDeviceExtensionSupport(vk::PhysicalDevice device, ugine::Span<const char*> deviceExtensions) {
    auto availableExtensions{ device.enumerateDeviceExtensionProperties() };
    std::set<std::string> requiredExtensions(deviceExtensions.Begin(), deviceExtensions.End());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: UGINE_TRACE(pCallbackData->pMessage); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: UGINE_INFO(pCallbackData->pMessage); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: UGINE_WARN(pCallbackData->pMessage); break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        UGINE_ERROR(pCallbackData->pMessage);
        //UGINE_BREAK;
        break;
    }

    return VK_FALSE;
}

vk::Result CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func{ (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT") };
    if (func) {
        func(instance, pCreateInfo, nullptr, pDebugMessenger);
        return vk::Result::eSuccess;
    }

    return vk::Result::eErrorExtensionNotPresent;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) {
    auto func{ (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT") };
    if (func) {
        func(instance, debugMessenger, nullptr);
    }
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =                          //
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | //
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | //
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =                             //
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |    //
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | //
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

bool IsDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR& surface, ugine::Span<const char*> deviceExtensions, DeviceDescriptor& outputDesciptor) {
    auto deviceProperties{ device.getProperties() };
    auto deviceFeatures{ device.getFeatures() };
    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    bool hasGfxCmdQueue{ false };
    bool hasComputeCmdQueue{ false };
    bool hasTransferCmdQueue{ false };
    bool supportSurfacePresent{ false };

    outputDesciptor.device = device;
    outputDesciptor.surface = surface;

    int i{};
    for (const auto& family : queueFamilies) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            hasGfxCmdQueue = true;
            outputDesciptor.queueFamilies.graphics = i;
        }

        if ((family.queueFlags & vk::QueueFlagBits::eCompute) && !(family.queueFlags & vk::QueueFlagBits::eGraphics)) {
            hasComputeCmdQueue = true;
            outputDesciptor.queueFamilies.compute = i;
        }

        if ((family.queueFlags & vk::QueueFlagBits::eTransfer) && !(family.queueFlags & vk::QueueFlagBits::eGraphics)
            && !(family.queueFlags & vk::QueueFlagBits::eCompute)) {
            hasTransferCmdQueue = true;
            outputDesciptor.queueFamilies.transfer = i;
        }

        vk::Bool32 presentSupport{ false };
        auto result{ device.getSurfaceSupportKHR(i, surface, &presentSupport) };

        if (result == vk::Result::eSuccess && presentSupport) {
            supportSurfacePresent = true;
            outputDesciptor.queueFamilies.surfacePresent = i;
        }

        ++i;

        if (hasGfxCmdQueue && supportSurfacePresent && hasComputeCmdQueue && hasTransferCmdQueue) {
            break;
        }
    }

    if (!hasTransferCmdQueue) {
        outputDesciptor.queueFamilies.transfer = outputDesciptor.queueFamilies.graphics;
    }

    if (!hasComputeCmdQueue) {
        outputDesciptor.queueFamilies.compute = outputDesciptor.queueFamilies.graphics;
    }

    const auto extensionsSupported{ CheckDeviceExtensionSupport(device, deviceExtensions) };

    bool swapChainAdequate{};
    if (extensionsSupported) {
        const auto swapChainSupport{ QuerySwapChainCapabilities(device, surface) };
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu // Is discrete GPU.
        && deviceFeatures.geometryShader                                       // Supports geometry shader.
        && hasGfxCmdQueue                                                      // Has graphics command queue.
        && supportSurfacePresent                                               // Has surface presentation queue.
        && extensionsSupported                                                 // Supports all required extensions.
        && swapChainAdequate;                                                  // Swap chain has all required capabilities.
}

const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

} // namespace

namespace ugine::gfxapi {

VulkanInstance::VulkanInstance(const DeviceCreateInfo& info)
    : enableValidationLayers_{ info.validationLayers } {

    Vector<const char*> instanceExtensions{ info.vulkan.instanceExtensions, info.vulkan.instanceExtensionsCount };

    vk::ApplicationInfo appInfo{
        .pApplicationName = info.vulkan.appName,
        .applicationVersion = VK_MAKE_VERSION(info.vulkan.appVer.maj, info.vulkan.appVer.min, info.vulkan.appVer.patch),
        .pEngineName = info.vulkan.engineName,
        .engineVersion = VK_MAKE_VERSION(info.vulkan.engineVer.maj, info.vulkan.engineVer.min, info.vulkan.engineVer.patch),
        .apiVersion = version_,
    };

    vk::InstanceCreateInfo createInfo{
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers_) {
        if (!CheckValidationLayerSupport()) {
            UGINE_THROW(GfxError, "Validation layers not available!");
        }

        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        createInfo.enabledLayerCount = u32(VALIDATION_LAYERS.size());

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    // Extensions.
    instanceExtensions.PushBack(VK_KHR_SURFACE_EXTENSION_NAME);
    instanceExtensions.PushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    CheckDebugSupport(instanceExtensions);
    if (enableValidationLayers_) {
        instanceExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        instanceExtensions.PushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

#ifdef WIN32
    // TODO: Make optional?
    instanceExtensions.PushBack(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

    createInfo.enabledExtensionCount = u32(instanceExtensions.Size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.Data();

    instance_ = vk::createInstanceUnique(createInfo);

    if (supportsDebug_ && enableValidationLayers_) {
        SetupDebugMessenger();
    }
}

VulkanInstance::~VulkanInstance() {
    if (debugMessenger_) {
        DestroyDebugUtilsMessengerEXT(*instance_, debugMessenger_);
    }
}

std::optional<DeviceDescriptor> VulkanInstance::FindFirstSuitableDevice(vk::SurfaceKHR surface, Span<const char*> deviceExtensions) {
    auto devices{ instance_->enumeratePhysicalDevices() };

    if (devices.empty()) {
        UGINE_ERROR("No suitable GPU device found.");
        return std::nullopt;
    }

    DeviceDescriptor deviceDesc{};
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device, surface, deviceExtensions, deviceDesc)) {
            return deviceDesc;
        }
    }

    UGINE_ERROR("No suitable GPU device found.");
    return std::nullopt;
}

bool VulkanInstance::CheckDebugSupport(Vector<const char*>& extensions) {
    u32 instance_extension_count;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr) == VK_SUCCESS) {

        std::vector<VkExtensionProperties> available_instance_extensions(instance_extension_count);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, available_instance_extensions.data()) == VK_SUCCESS) {

            for (auto& available_extension : available_instance_extensions) {
                if (strcmp(available_extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                    supportsDebug_ = true;
                    extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                }
            }
        }
    }

    return supportsDebug_;
}

bool VulkanInstance::CheckValidationLayerSupport() const {
    auto availableLayers{ vk::enumerateInstanceLayerProperties() };
    for (const char* layerName : VALIDATION_LAYERS) {
        auto layerFound{ false };

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void VulkanInstance::SetupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    PopulateDebugMessengerCreateInfo(createInfo);

    auto result{ CreateDebugUtilsMessengerEXT(*instance_, &createInfo, &debugMessenger_) };
    if (result != vk::Result::eSuccess) {
        //UGINE_THROW(GfxError, "Failed to create debug messenger.", result);
        UGINE_ERROR("Failed to create debug messenger: {}", int(result));
    }
}

} // namespace ugine::gfxapi
