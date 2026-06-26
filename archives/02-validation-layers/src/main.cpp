#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::array validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

void check(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(message) + " VkResult=" + std::to_string(result));
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData) {
    (void)messageSeverity;
    (void)messageType;
    (void)userData;

    // 验证层会把参数错误、对象生命周期问题和潜在性能问题传到这里。
    // 当前阶段先统一输出到标准错误流，后续可以根据严重程度做过滤。
    std::cerr << "[Vulkan validation] " << callbackData->pMessage << '\n';
    return VK_FALSE;
}

bool checkValidationLayerSupport() {
    uint32_t layerCount = 0;
    check(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), "Failed to count Vulkan instance layers.");

    std::vector<VkLayerProperties> availableLayers(layerCount);
    check(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()), "Failed to enumerate Vulkan instance layers.");

    for (const char* requiredLayer : validationLayers) {
        bool found = false;
        for (const auto& layer : availableLayers) {
            if (std::strcmp(requiredLayer, layer.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> getRequiredExtensions() {
    std::vector<const char*> extensions;

#ifndef NDEBUG
    // VkDebugUtilsMessengerEXT 属于 VK_EXT_debug_utils 扩展。
    // 启用验证层时必须把这个实例扩展一并打开，之后才能创建调试回调对象。
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

VkResult createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debugMessenger) {
    // 扩展函数不会直接暴露为普通 Vulkan 函数指针，需要通过 vkGetInstanceProcAddr 从实例中查询。
    // 这也说明调试回调的生命周期依赖 VkInstance，销毁实例前必须先销毁它。
    const auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (function == nullptr) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    return function(instance, createInfo, allocator, debugMessenger);
}

void destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* allocator) {
    const auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (function != nullptr) {
        function(instance, debugMessenger, allocator);
    }
}

VkInstance createInstance() {
#ifndef NDEBUG
    if (!checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers were requested, but VK_LAYER_KHRONOS_validation is not available.");
    }
#endif

    // VkApplicationInfo 用来告诉驱动这个程序和引擎的基本信息。
    // 这些信息不直接决定功能，但有助于驱动和调试工具识别应用。
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Learn Vulkan Validation Layers";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const auto extensions = getRequiredExtensions();

    // VkInstance 是 Vulkan 程序最外层的上下文对象。
    // 第二阶段在创建实例时额外声明验证层和调试扩展，让驱动加载诊断工具链。
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    (void)debugCreateInfo;

#ifndef NDEBUG
    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    // 把调试回调挂到 pNext，可以捕捉 vkCreateInstance 和 vkDestroyInstance 附近的验证层消息。
    // 正式的 VkDebugUtilsMessengerEXT 会在实例创建成功后再单独创建。
    createInfo.pNext = &debugCreateInfo;
#endif

    VkInstance instance = VK_NULL_HANDLE;
    check(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create Vulkan instance.");
    return instance;
}

VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance) {
#ifdef NDEBUG
    (void)instance;
    return VK_NULL_HANDLE;
#else
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    check(createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger), "Failed to set up debug messenger.");
    return debugMessenger;
#endif
}

std::vector<VkPhysicalDevice> enumeratePhysicalDevices(VkInstance instance) {
    // Vulkan 常见的枚举模式是先用 nullptr 查询数量，再分配数组获取具体对象。
    uint32_t deviceCount = 0;
    check(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr), "Failed to count Vulkan devices.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (deviceCount > 0) {
        check(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()), "Failed to enumerate Vulkan devices.");
    }

    return devices;
}

} // namespace

int main() {
    try {
        // 先创建 VkInstance，再创建依赖实例的调试回调，最后枚举物理设备。
        // 销毁时要倒过来：先销毁调试回调，再销毁 VkInstance。
        const VkInstance instance = createInstance();
        const VkDebugUtilsMessengerEXT debugMessenger = setupDebugMessenger(instance);
        const auto devices = enumeratePhysicalDevices(instance);

        std::cout << "Hello, Vulkan!" << '\n';
        std::cout << "Validation layers: " << (enableValidationLayers ? "enabled" : "disabled") << '\n';
        std::cout << "Found " << devices.size() << " Vulkan device(s)." << '\n';

        for (size_t i = 0; i < devices.size(); ++i) {
            // VkPhysicalDeviceProperties 保存 GPU 名称、类型、驱动版本和 API 版本等信息。
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(devices[i], &properties);

            std::cout << "GPU " << i << ": " << properties.deviceName
                      << " | API "
                      << VK_VERSION_MAJOR(properties.apiVersion) << '.'
                      << VK_VERSION_MINOR(properties.apiVersion) << '.'
                      << VK_VERSION_PATCH(properties.apiVersion) << '\n';
        }

        if (debugMessenger != VK_NULL_HANDLE) {
            destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroyInstance(instance, nullptr);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
