#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr uint32_t windowWidth = 800;
constexpr uint32_t windowHeight = 600;

constexpr std::array validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

constexpr std::array deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

void check(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(message) + " VkResult=" + std::to_string(result));
    }
}

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        throw std::runtime_error("Shader file is empty: " + filename);
    }

    std::vector<char> buffer(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    if (!file) {
        throw std::runtime_error("Failed to read file: " + filename);
    }

    return buffer;
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
    // 第七阶段开始创建图形管线，Shader Module、Render Pass 兼容性和对象销毁顺序的问题很适合交给验证层提醒。
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
    // Surface 是 Vulkan 和窗口系统之间的桥梁。
    // 不同平台需要不同实例扩展，GLFW 会根据当前系统返回正确的扩展列表。
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions == nullptr || glfwExtensionCount == 0) {
        throw std::runtime_error("GLFW did not report the required Vulkan instance extensions.");
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

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

class VulkanWindowApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
    }

    ~VulkanWindowApplication() {
        cleanup();
    }

    VulkanWindowApplication(const VulkanWindowApplication&) = delete;
    VulkanWindowApplication& operator=(const VulkanWindowApplication&) = delete;

    VulkanWindowApplication() = default;

private:
    GLFWwindow* window_ = nullptr;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages_;
    VkFormat swapChainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent_{};
    std::vector<VkImageView> swapChainImageViews_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    QueueFamilyIndices selectedQueueFamilies_{};

    void initWindow() {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Failed to initialize GLFW.");
        }

        if (glfwVulkanSupported() != GLFW_TRUE) {
            throw std::runtime_error("GLFW reports that Vulkan is not supported on this system.");
        }

        // GLFW 默认会创建 OpenGL 上下文；本项目只需要一个承载 Vulkan Surface 的窗口。
        // 关闭客户端 API 可以避免 GLFW 额外初始化 OpenGL，让窗口专门服务 Vulkan。
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window_ = glfwCreateWindow(windowWidth, windowHeight, "Learn Vulkan - Graphics Pipeline", nullptr, nullptr);
        if (window_ == nullptr) {
            throw std::runtime_error("Failed to create GLFW window.");
        }
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        printSelectedDeviceInfo();
    }

    void createInstance() {
#ifndef NDEBUG
        if (!checkValidationLayerSupport()) {
            throw std::runtime_error("Validation layers were requested, but VK_LAYER_KHRONOS_validation is not available.");
        }
#endif

        // VkApplicationInfo 用来告诉驱动这个程序和引擎的基本信息。
        // 这些信息不直接决定功能，但有助于驱动和调试工具识别应用。
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Learn Vulkan Graphics Pipeline";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        const auto extensions = getRequiredExtensions();

        // VkInstance 是 Vulkan 程序最外层的上下文对象。
        // Surface 扩展必须在实例创建前声明，否则后面无法创建窗口呈现目标。
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

        check(vkCreateInstance(&createInfo, nullptr, &instance_), "Failed to create Vulkan instance.");
    }

    void setupDebugMessenger() {
#ifndef NDEBUG
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populateDebugMessengerCreateInfo(createInfo);

        check(createDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_), "Failed to set up debug messenger.");
#endif
    }

    void createSurface() {
        // VkSurfaceKHR 表示 Vulkan 眼里的“窗口显示目标”。
        // Swapchain 会基于它创建一组可呈现到窗口的图像。
        check(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_), "Failed to create Vulkan window surface.");
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const {
        QueueFamilyIndices indices;

        // 队列族是一组能力相同的队列。图形命令、计算命令、传输命令和窗口呈现可能由不同队列族支持。
        // 本阶段继续要求一个队列族能处理图形命令，另一个队列族能把 Swapchain 图像呈现到当前 Surface。
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            check(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport), "Failed to query queue present support.");
            if (presentSupport == VK_TRUE) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const {
        uint32_t extensionCount = 0;
        check(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr), "Failed to count Vulkan device extensions.");

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        check(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()), "Failed to enumerate Vulkan device extensions.");

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const {
        SwapChainSupportDetails details;

        // Surface 能力描述了窗口系统允许的交换链边界，例如图像数量、尺寸范围和变换方式。
        // 这些限制由设备、驱动、窗口系统共同决定，因此必须针对当前物理设备和 Surface 查询。
        check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities), "Failed to query surface capabilities.");

        uint32_t formatCount = 0;
        check(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr), "Failed to count surface formats.");
        if (formatCount > 0) {
            details.formats.resize(formatCount);
            check(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data()), "Failed to query surface formats.");
        }

        uint32_t presentModeCount = 0;
        check(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr), "Failed to count present modes.");
        if (presentModeCount > 0) {
            details.presentModes.resize(presentModeCount);
            check(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data()), "Failed to query present modes.");
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) const {
        const QueueFamilyIndices indices = findQueueFamilies(device);
        const bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    void pickPhysicalDevice() {
        // VkPhysicalDevice 代表系统中的真实 GPU 或 Vulkan 兼容设备。
        // 第七阶段仍然需要交换链作为图形管线的输出目标，因此设备必须支持 VK_KHR_swapchain 扩展和可用的 Surface 配置。
        uint32_t deviceCount = 0;
        check(vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr), "Failed to count Vulkan devices.");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        if (deviceCount > 0) {
            check(vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data()), "Failed to enumerate Vulkan devices.");
        }

        std::cout << "Hello, Vulkan!" << '\n';
        std::cout << "Validation layers: " << (enableValidationLayers ? "enabled" : "disabled") << '\n';
        std::cout << "Window surface: created" << '\n';
        std::cout << "Found " << devices.size() << " Vulkan device(s)." << '\n';

        for (VkPhysicalDevice device : devices) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);

            const QueueFamilyIndices indices = findQueueFamilies(device);
            const bool extensionsSupported = checkDeviceExtensionSupport(device);
            const SwapChainSupportDetails swapChainSupport = extensionsSupported ? querySwapChainSupport(device) : SwapChainSupportDetails{};

            std::cout << "GPU: " << properties.deviceName
                      << " | API "
                      << VK_VERSION_MAJOR(properties.apiVersion) << '.'
                      << VK_VERSION_MINOR(properties.apiVersion) << '.'
                      << VK_VERSION_PATCH(properties.apiVersion)
                      << " | graphics queue: "
                      << (indices.graphicsFamily.has_value() ? std::to_string(indices.graphicsFamily.value()) : "not found")
                      << " | present queue: "
                      << (indices.presentFamily.has_value() ? std::to_string(indices.presentFamily.value()) : "not found")
                      << " | swapchain extension: " << (extensionsSupported ? "yes" : "no")
                      << " | formats: " << swapChainSupport.formats.size()
                      << " | present modes: " << swapChainSupport.presentModes.size()
                      << '\n';

            if (physicalDevice_ == VK_NULL_HANDLE && isDeviceSuitable(device)) {
                physicalDevice_ = device;
                selectedQueueFamilies_ = indices;
            }
        }

        if (physicalDevice_ == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a GPU with graphics, present, and swapchain support.");
        }
    }

    void createLogicalDevice() {
        // VkDevice 是从物理设备创建出来的逻辑设备，后续大多数 Vulkan 对象都会挂在它下面。
        // 创建逻辑设备时必须启用 VK_KHR_swapchain，否则设备级交换链函数不可用。
        const std::set<uint32_t> uniqueQueueFamilies = {
            selectedQueueFamilies_.graphicsFamily.value(),
            selectedQueueFamilies_.presentFamily.value(),
        };

        constexpr float queuePriority = 1.0F;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        queueCreateInfos.reserve(uniqueQueueFamilies.size());

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

#ifndef NDEBUG
        // Vulkan 1.0 的旧实现会从逻辑设备创建信息中读取验证层列表。
        // 新实现主要使用实例级验证层，但保留这段配置有助于兼容和学习对象层级。
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
#endif

        check(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_), "Failed to create logical device.");

        // VkQueue 由逻辑设备持有，不能手动销毁；它会随着 VkDevice 一起释放。
        // 这里保存图形队列和呈现队列句柄，后续提交绘制命令和呈现图像时会用到它们。
        vkGetDeviceQueue(device_, selectedQueueFamilies_.graphicsFamily.value(), 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, selectedQueueFamilies_.presentFamily.value(), 0, &presentQueue_);
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
        // BGRA + sRGB 是常见窗口颜色格式，适合后续直接输出到屏幕。
        // 如果驱动没有提供这个组合，就退回到第一个可用格式，保持程序能继续运行。
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats.front();
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const {
        // MAILBOX 类似三缓冲，延迟较低且不会撕裂；如果不可用，FIFO 是 Vulkan 规范保证存在的安全选择。
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        // 有些平台会让应用自己选择交换链尺寸，此时应该使用真实 framebuffer 尺寸而不是逻辑窗口尺寸。
        // 高 DPI 屏幕上二者可能不同，GLFW 的 framebuffer size 更接近 Vulkan 图像需要的像素尺寸。
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window_, &width, &height);

        VkExtent2D actualExtent{
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }

    void createSwapChain() {
        const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice_);

        const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        const VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // 通常比最小图像数量多申请一张，这样 CPU/GPU 可以更从容地轮换图像。
        // 如果窗口系统设置了 maxImageCount，就必须尊重上限。
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface_;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const std::array queueFamilyIndices = {
            selectedQueueFamilies_.graphicsFamily.value(),
            selectedQueueFamilies_.presentFamily.value(),
        };

        if (selectedQueueFamilies_.graphicsFamily != selectedQueueFamilies_.presentFamily) {
            // 如果图形和呈现来自不同队列族，使用 CONCURRENT 可以让交换链图像同时被两个队列族访问。
            // 这样代码更简单；后续追求性能时可以再学习 EXCLUSIVE 和显式所有权转移。
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        check(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_), "Failed to create swapchain.");

        // Swapchain 图像由交换链拥有，应用只获取句柄用于创建 Image View，不需要也不能单独销毁这些图像。
        check(vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr), "Failed to count swapchain images.");
        swapChainImages_.resize(imageCount);
        check(vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data()), "Failed to get swapchain images.");

        swapChainImageFormat_ = surfaceFormat.format;
        swapChainExtent_ = extent;
    }

    void createImageViews() {
        // VkImage 是实际图像资源，VkImageView 是 shader 和渲染流程访问图像时使用的“视图”。
        // Swapchain 图像由窗口系统创建，我们不能改变图像本身，但需要为每张图像创建一个颜色视图。
        swapChainImageViews_.resize(swapChainImages_.size());

        for (size_t i = 0; i < swapChainImages_.size(); ++i) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages_[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat_;

            // components 用来重新映射颜色通道。本阶段不做通道交换，保持 RGBA 原样传递。
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // subresourceRange 说明这个视图覆盖图像的哪些部分。
            // Swapchain 图像只作为颜色附件使用，所以 aspectMask 选择 COLOR，且只需要第 0 层 mip 和第 0 层数组。
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            check(vkCreateImageView(device_, &createInfo, nullptr, &swapChainImageViews_[i]), "Failed to create swapchain image view.");
        }
    }

    void createRenderPass() {
        // Render Pass 描述一次渲染会使用哪些附件，以及这些附件在渲染前后处于什么布局。
        // 本阶段只有一个颜色附件，它最终会被呈现到窗口，所以 finalLayout 选择 PRESENT_SRC_KHR。
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat_;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Attachment Reference 把 Render Pass 的附件列表映射到 Subpass 中的具体用途。
        // layout 表示这个附件在子通道执行期间应处于颜色附件最合适的布局。
        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.attachment = 0;
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Subpass 是 Render Pass 内部的一次渲染步骤。
        // 当前只需要一个图形子通道，把输出颜色写入第 0 个颜色附件。
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentReference;

        // 依赖关系告诉 Vulkan：外部呈现/获取图像的访问要和本次颜色附件写入正确衔接。
        // 这里先建立最基础的同步关系，完整的帧同步会在第九阶段展开。
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        check(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_), "Failed to create render pass.");
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) const {
        // Shader Module 是驱动可识别的 SPIR-V 二进制着色器对象。
        // 它只在创建图形管线时需要；管线创建成功后，模块本身就可以销毁。
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;
        check(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule), "Failed to create shader module.");
        return shaderModule;
    }

    void createGraphicsPipeline() {
        const std::string shaderDirectory = SHADER_DIR;
        const std::vector<char> vertexShaderCode = readFile(shaderDirectory + "/triangle.vert.spv");
        const std::vector<char> fragmentShaderCode = readFile(shaderDirectory + "/triangle.frag.spv");

        const VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
        const VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";

        const std::array shaderStages = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo,
        };

        // 顶点输入阶段描述 Vertex Buffer 的布局。
        // 本阶段的顶点 shader 使用 gl_VertexIndex 生成三角形，所以这里保持空配置。
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0F;
        viewport.y = 0.0F;
        viewport.width = static_cast<float>(swapChainExtent_.width);
        viewport.height = static_cast<float>(swapChainExtent_.height);
        viewport.minDepth = 0.0F;
        viewport.maxDepth = 1.0F;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent_;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0F;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // Pipeline Layout 描述 shader 会访问哪些 Descriptor Set 和 Push Constant。
        // 当前 shader 没有外部资源输入，所以创建一个空布局即可。
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        check(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_), "Failed to create pipeline layout.");

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.renderPass = renderPass_;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        check(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_), "Failed to create graphics pipeline.");

        vkDestroyShaderModule(device_, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(device_, vertexShaderModule, nullptr);
    }

    void printSelectedDeviceInfo() const {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice_, &properties);

        std::cout << "Selected GPU: " << properties.deviceName << '\n';
        std::cout << "Logical device: created" << '\n';
        std::cout << "Graphics queue family: " << selectedQueueFamilies_.graphicsFamily.value() << '\n';
        std::cout << "Present queue family: " << selectedQueueFamilies_.presentFamily.value() << '\n';
        std::cout << "Swapchain: created" << '\n';
        std::cout << "Swapchain image count: " << swapChainImages_.size() << '\n';
        std::cout << "Swapchain image view count: " << swapChainImageViews_.size() << '\n';
        std::cout << "Swapchain format: " << swapChainImageFormat_ << '\n';
        std::cout << "Swapchain extent: " << swapChainExtent_.width << "x" << swapChainExtent_.height << '\n';
        std::cout << "Render pass: created" << '\n';
        std::cout << "Graphics pipeline: created" << '\n';
        std::cout << std::flush;
    }

    void mainLoop() const {
        // 现在已经有图形管线，但还没有 Framebuffer 和命令录制。
        // 窗口循环仍然只负责处理事件，真正提交队列的绘制流程会在后续阶段加入。
        while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        // 图形管线和 Pipeline Layout 都是设备级对象，要在 Render Pass 和 VkDevice 销毁前释放。
        if (graphicsPipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
            graphicsPipeline_ = VK_NULL_HANDLE;
        }

        if (pipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
            pipelineLayout_ = VK_NULL_HANDLE;
        }

        // Render Pass 由逻辑设备创建，后续 Pipeline 和 Framebuffer 会依赖它。
        // 当前阶段退出时要在销毁 VkDevice 前释放。
        if (renderPass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, renderPass_, nullptr);
            renderPass_ = VK_NULL_HANDLE;
        }

        // Image View 是应用为 Swapchain 图像创建的设备级对象。
        // 它们依赖 Swapchain 图像，因此要在销毁 Swapchain 之前逐个释放。
        for (VkImageView imageView : swapChainImageViews_) {
            vkDestroyImageView(device_, imageView, nullptr);
        }
        swapChainImageViews_.clear();

        // Swapchain 依赖逻辑设备和 Surface，必须在 VkDevice 和 VkSurfaceKHR 销毁前释放。
        // Swapchain 图像由交换链统一管理，销毁交换链时会一起结束生命周期。
        if (swapChain_ != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device_, swapChain_, nullptr);
            swapChain_ = VK_NULL_HANDLE;
        }

        if (device_ != VK_NULL_HANDLE) {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }

        if (surface_ != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance_, surface_, nullptr);
            surface_ = VK_NULL_HANDLE;
        }

        if (debugMessenger_ != VK_NULL_HANDLE) {
            destroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
            debugMessenger_ = VK_NULL_HANDLE;
        }

        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }

        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }

        glfwTerminate();
    }
};

} // namespace

int main() {
    try {
        VulkanWindowApplication application;
        application.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
