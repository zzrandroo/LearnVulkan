#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
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
    // 第四阶段开始创建逻辑设备，设备和队列的生命周期错误也会通过这个回调暴露出来。
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

        window_ = glfwCreateWindow(windowWidth, windowHeight, "Learn Vulkan - Logical Device Queues", nullptr, nullptr);
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
        appInfo.pApplicationName = "Learn Vulkan Logical Device Queues";
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
        // 它依赖实例和窗口，第四阶段会用它检查 GPU 队列是否能把图像呈现到这个窗口。
        check(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_), "Failed to create Vulkan window surface.");
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const {
        QueueFamilyIndices indices;

        // 队列族是一组能力相同的队列。图形命令、计算命令、传输命令和窗口呈现可能由不同队列族支持。
        // 本阶段只要求一个队列族能处理图形命令，另一个队列族能把 Swapchain 图像呈现到当前 Surface。
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

    bool isDeviceSuitable(VkPhysicalDevice device) const {
        const QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete();
    }

    void pickPhysicalDevice() {
        // VkPhysicalDevice 代表系统中的真实 GPU 或 Vulkan 兼容设备。
        // 只有当它提供我们需要的队列族时，后续才能基于它创建逻辑设备。
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
            std::cout << "GPU: " << properties.deviceName
                      << " | API "
                      << VK_VERSION_MAJOR(properties.apiVersion) << '.'
                      << VK_VERSION_MINOR(properties.apiVersion) << '.'
                      << VK_VERSION_PATCH(properties.apiVersion)
                      << " | graphics queue: "
                      << (indices.graphicsFamily.has_value() ? std::to_string(indices.graphicsFamily.value()) : "not found")
                      << " | present queue: "
                      << (indices.presentFamily.has_value() ? std::to_string(indices.presentFamily.value()) : "not found")
                      << '\n';

            if (physicalDevice_ == VK_NULL_HANDLE && isDeviceSuitable(device)) {
                physicalDevice_ = device;
                selectedQueueFamilies_ = indices;
            }
        }

        if (physicalDevice_ == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a GPU with graphics and present queue support.");
        }
    }

    void createLogicalDevice() {
        // VkDevice 是从物理设备创建出来的逻辑设备，后续大多数 Vulkan 对象都会挂在它下面。
        // 创建逻辑设备时要明确向哪些队列族申请队列，否则之后拿不到可提交命令的 VkQueue。
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

    void printSelectedDeviceInfo() const {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice_, &properties);

        std::cout << "Selected GPU: " << properties.deviceName << '\n';
        std::cout << "Logical device: created" << '\n';
        std::cout << "Graphics queue family: " << selectedQueueFamilies_.graphicsFamily.value() << '\n';
        std::cout << "Present queue family: " << selectedQueueFamilies_.presentFamily.value() << '\n';
        std::cout << std::flush;
    }

    void mainLoop() const {
        // 现在已经有逻辑设备和队列，但还没有 Swapchain 和渲染命令。
        // 窗口循环仍然只负责处理事件，真正提交队列的绘制流程会在后续阶段加入。
        while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        // 逻辑设备依赖被选中的物理设备，物理设备本身不需要销毁。
        // 在释放 Surface 和 Instance 之前先销毁 VkDevice，确保设备级对象和队列都已经结束生命周期。
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
