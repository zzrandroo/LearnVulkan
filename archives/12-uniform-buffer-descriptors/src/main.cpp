#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
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
constexpr uint64_t fenceTimeout = std::numeric_limits<uint64_t>::max();
constexpr float pi = 3.14159265358979323846F;

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

struct Vertex {
    float position[2];
    float color[3];

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

constexpr std::array vertices = {
    Vertex{{0.0F, -0.5F}, {1.0F, 0.1F, 0.1F}},
    Vertex{{0.5F, 0.5F}, {0.1F, 1.0F, 0.1F}},
    Vertex{{-0.5F, 0.5F}, {0.1F, 0.2F, 1.0F}},
};

constexpr std::array<uint16_t, 3> triangleIndices = {
    0,
    1,
    2,
};

struct UniformBufferObject {
    alignas(16) std::array<float, 16> mvp{};
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

UniformBufferObject makeUniformBufferObject(float seconds) {
    const float angle = seconds * pi * 0.5F;
    const float c = std::cos(angle);
    const float s = std::sin(angle);

    UniformBufferObject ubo{};
    // GLSL 默认按列主序读取 mat4。这个矩阵只做 Z 轴旋转，便于观察 Uniform Buffer 每帧更新的效果。
    ubo.mvp = {
        c, s, 0.0F, 0.0F,
        -s, c, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    };
    return ubo;
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
    // 第十二阶段开始使用 Uniform Buffer 和 Descriptor Set，资源绑定与 shader 接口的问题很适合交给验证层提醒。
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
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer uniformBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory_ = VK_NULL_HANDLE;
    void* uniformBufferMapped_ = nullptr;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers_;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;
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

        window_ = glfwCreateWindow(windowWidth, windowHeight, "Learn Vulkan - Uniform Buffer Descriptors", nullptr, nullptr);
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
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffer();
        createDescriptorPool();
        createDescriptorSet();
        createFramebuffers();
        createCommandBuffers();
        createSyncObjects();
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
        appInfo.pApplicationName = "Learn Vulkan Uniform Buffer Descriptors";
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
        // 第十二阶段仍然需要交换链作为每帧呈现目标，因此设备必须支持 VK_KHR_swapchain 扩展和可用的 Surface 配置。
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

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            const bool typeMatches = (typeFilter & (1U << i)) != 0;
            const bool hasProperties = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
            if (typeMatches && hasProperties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type.");
    }

    void createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory) const {
        // Buffer 只是一个资源句柄，真正的显存或系统内存要通过 VkDeviceMemory 分配后再绑定。
        // Vulkan 把资源描述和内存分配拆开，是为了让应用能精确控制内存类型和复用策略。
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        check(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer), "Failed to create buffer.");

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(device_, buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

        check(vkAllocateMemory(device_, &allocateInfo, nullptr, &bufferMemory), "Failed to allocate buffer memory.");
        check(vkBindBufferMemory(device_, buffer, bufferMemory, 0), "Failed to bind buffer memory.");
    }

    VkCommandBuffer beginSingleTimeCommands() const {
        // 一次性命令缓冲适合资源上传这样的短命令。
        // 它会从已有 Command Pool 分配，录制一次 copy 后立刻提交并等待完成。
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = commandPool_;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        check(vkAllocateCommandBuffers(device_, &allocateInfo, &commandBuffer), "Failed to allocate single time command buffer.");

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin single time command buffer.");
        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
        check(vkEndCommandBuffer(commandBuffer), "Failed to end single time command buffer.");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        check(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit single time command buffer.");
        check(vkQueueWaitIdle(graphicsQueue_), "Failed to wait for graphics queue idle.");

        vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size) const {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void createVertexBuffer() {
        const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        // Staging Buffer 使用 CPU 可见内存，专门负责把顶点数据写进 Vulkan。
        // 真正绘制时使用 Device Local 的 vertexBuffer_，让 GPU 读取更合适的内存位置。
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);

        void* data = nullptr;
        check(vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data), "Failed to map vertex staging buffer memory.");
        std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device_, stagingBufferMemory);

        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBuffer_,
            vertexBufferMemory_);

        copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);

        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        const VkDeviceSize bufferSize = sizeof(triangleIndices[0]) * triangleIndices.size();

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);

        void* data = nullptr;
        check(vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data), "Failed to map index staging buffer memory.");
        std::memcpy(data, triangleIndices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device_, stagingBufferMemory);

        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            indexBuffer_,
            indexBufferMemory_);

        copyBuffer(stagingBuffer, indexBuffer_, bufferSize);

        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);
    }

    void createUniformBuffer() {
        const VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        // Uniform Buffer 体积很小且每帧更新，使用 Host Visible + Host Coherent 内存能让代码保持直接。
        // Descriptor Set 会把这个 Buffer 绑定到 shader 的 binding 0。
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffer_,
            uniformBufferMemory_);

        check(vkMapMemory(device_, uniformBufferMemory_, 0, bufferSize, 0, &uniformBufferMapped_), "Failed to map uniform buffer memory.");
    }

    void createDescriptorPool() {
        // Descriptor Pool 用来分配 Descriptor Set。
        // 当前只有一个 Uniform Buffer 描述符，因此池大小也保持最小。
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        check(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_), "Failed to create descriptor pool.");
    }

    void createDescriptorSet() {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool_;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &descriptorSetLayout_;

        check(vkAllocateDescriptorSets(device_, &allocateInfo, &descriptorSet_), "Failed to allocate descriptor set.");

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer_;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet_;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
    }

    void updateUniformBuffer() const {
        static const auto startTime = std::chrono::steady_clock::now();
        const auto currentTime = std::chrono::steady_clock::now();
        const float seconds = std::chrono::duration<float>(currentTime - startTime).count();

        const UniformBufferObject ubo = makeUniformBufferObject(seconds);
        std::memcpy(uniformBufferMapped_, &ubo, sizeof(ubo));
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
        // 这里建立颜色附件写入所需的基础依赖，帧级同步由 Semaphore 和 Fence 负责。
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

    void createDescriptorSetLayout() {
        // Descriptor Set Layout 是 shader 资源接口的声明。
        // binding 0 对应顶点 shader 中的 uniform UniformBufferObject，用来读取每帧更新的 MVP 矩阵。
        VkDescriptorSetLayoutBinding uniformBufferLayoutBinding{};
        uniformBufferLayoutBinding.binding = 0;
        uniformBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayoutBinding.descriptorCount = 1;
        uniformBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uniformBufferLayoutBinding;

        check(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_), "Failed to create descriptor set layout.");
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

        // 顶点输入阶段描述 Vertex Buffer 的内存布局。
        // binding 描述每个顶点占多少字节，attribute 描述 shader location 如何从顶点结构中取数据。
        const auto bindingDescription = Vertex::getBindingDescription();
        const auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
        // 第十二阶段把 Uniform Buffer 的 Descriptor Set Layout 接入管线布局。
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;

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

    void createFramebuffers() {
        // Framebuffer 把 Render Pass 中声明的附件，绑定到某一组真实的 Image View。
        // Swapchain 有几张图像，就需要几个 Framebuffer，后续每帧会选择当前获得的那一个来渲染。
        swapChainFramebuffers_.resize(swapChainImageViews_.size());

        for (size_t i = 0; i < swapChainImageViews_.size(); ++i) {
            const std::array attachments = {
                swapChainImageViews_[i],
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass_;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent_.width;
            framebufferInfo.height = swapChainExtent_.height;
            framebufferInfo.layers = 1;

            check(vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[i]), "Failed to create framebuffer.");
        }
    }

    void createCommandPool() {
        // Command Pool 负责分配 Command Buffer，并且绑定到一个队列族。
        // 本阶段录制图形命令，所以使用图形队列族；后续提交时也会提交到 graphicsQueue_。
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = selectedQueueFamilies_.graphicsFamily.value();

        check(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "Failed to create command pool.");
    }

    void createCommandBuffers() {
        // Command Buffer 记录的是将来要交给 GPU 队列执行的命令。
        // 录制命令不会立刻绘制，只有提交到队列之后 GPU 才会真正执行这些命令。
        commandBuffers_.resize(swapChainFramebuffers_.size());

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool_;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

        check(vkAllocateCommandBuffers(device_, &allocateInfo, commandBuffers_.data()), "Failed to allocate command buffers.");

        for (size_t i = 0; i < commandBuffers_.size(); ++i) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            check(vkBeginCommandBuffer(commandBuffers_[i], &beginInfo), "Failed to begin recording command buffer.");

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass_;
            renderPassInfo.framebuffer = swapChainFramebuffers_[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent_;

            const std::array clearValues = {
                VkClearValue{{{0.04F, 0.06F, 0.10F, 1.0F}}},
            };
            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffers_[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

            const std::array vertexBuffers = {
                vertexBuffer_,
            };
            const std::array<VkDeviceSize, 1> offsets = {
                0,
            };
            vkCmdBindVertexBuffers(commandBuffers_[i], 0, static_cast<uint32_t>(vertexBuffers.size()), vertexBuffers.data(), offsets.data());
            vkCmdBindIndexBuffer(commandBuffers_[i], indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindDescriptorSets(
                commandBuffers_[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout_,
                0,
                1,
                &descriptorSet_,
                0,
                nullptr);

            // 索引缓冲决定顶点复用和绘制顺序，GPU 会根据 indexBuffer_ 中的索引去读取 Vertex Buffer。
            // 当前仍然绘制一个三角形，但命令已经切换为后续模型绘制更常用的 vkCmdDrawIndexed。
            vkCmdDrawIndexed(commandBuffers_[i], static_cast<uint32_t>(triangleIndices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffers_[i]);

            check(vkEndCommandBuffer(commandBuffers_[i]), "Failed to record command buffer.");
        }
    }

    void createSyncObjects() {
        // Semaphore 用于 GPU 队列之间的等待关系：图像可用后才能渲染，渲染结束后才能呈现。
        // Fence 用于 CPU 等待 GPU：避免 CPU 在上一帧还没执行完时就重用同一批同步对象。
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        check(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphore_), "Failed to create image available semaphore.");
        check(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphore_), "Failed to create render finished semaphore.");
        check(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFence_), "Failed to create in-flight fence.");
    }

    void drawFrame() const {
        // 等待上一帧提交的 GPU 工作完成，再复用本阶段这唯一一组同步对象。
        // 当前仍然保持单帧在飞，理解清楚后再扩展到多帧并行。
        check(vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, fenceTimeout), "Failed to wait for in-flight fence.");

        uint32_t imageIndex = 0;
        const VkResult acquireResult = vkAcquireNextImageKHR(
            device_,
            swapChain_,
            fenceTimeout,
            imageAvailableSemaphore_,
            VK_NULL_HANDLE,
            &imageIndex);

        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
            throw std::runtime_error("Swapchain is out of date. Swapchain recreation will be handled in a later stage.");
        }
        if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swapchain image. VkResult=" + std::to_string(acquireResult));
        }

        updateUniformBuffer();

        check(vkResetFences(device_, 1, &inFlightFence_), "Failed to reset in-flight fence.");

        const std::array waitSemaphores = {
            imageAvailableSemaphore_,
        };
        const std::array<VkPipelineStageFlags, 1> waitStages = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
        const std::array signalSemaphores = {
            renderFinishedSemaphore_,
        };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        // vkQueueSubmit 才是真正把录制好的 Command Buffer 交给 GPU 队列执行的地方。
        // inFlightFence 会在这批 GPU 工作完成后变为 signaled，供下一轮 CPU 等待。
        check(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFence_), "Failed to submit draw command buffer.");

        const std::array swapChains = {
            swapChain_,
        };

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        presentInfo.pWaitSemaphores = signalSemaphores.data();
        presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
        presentInfo.pSwapchains = swapChains.data();
        presentInfo.pImageIndices = &imageIndex;

        const VkResult presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
            throw std::runtime_error("Swapchain became out of date during present. Swapchain recreation will be handled in a later stage.");
        }
        if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to present swapchain image. VkResult=" + std::to_string(presentResult));
        }
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
        std::cout << "Vertex buffer: created with " << vertices.size() << " vertices" << '\n';
        std::cout << "Index buffer: created with " << triangleIndices.size() << " indices" << '\n';
        std::cout << "Uniform buffer and descriptor set: created" << '\n';
        std::cout << "Framebuffer count: " << swapChainFramebuffers_.size() << '\n';
        std::cout << "Command buffers recorded: " << commandBuffers_.size() << '\n';
        std::cout << "Sync objects: created" << '\n';
        std::cout << std::flush;
    }

    void mainLoop() const {
        // 每轮窗口事件后提交一帧：获取交换链图像、提交图形命令、等待渲染完成后呈现。
        // 这就是最小 draw frame 流程；更健壮的窗口缩放和多帧并行会在后续阶段补充。
        while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
            glfwPollEvents();
            drawFrame();
        }

        // 退出前等待 GPU 空闲，避免清理资源时 GPU 仍然持有 Command Buffer、Framebuffer 或 Swapchain 图像。
        check(vkDeviceWaitIdle(device_), "Failed to wait for device idle.");
    }

    void cleanup() {
        if (device_ != VK_NULL_HANDLE) {
            (void)vkDeviceWaitIdle(device_);
        }

        if (inFlightFence_ != VK_NULL_HANDLE) {
            vkDestroyFence(device_, inFlightFence_, nullptr);
            inFlightFence_ = VK_NULL_HANDLE;
        }

        if (renderFinishedSemaphore_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
            renderFinishedSemaphore_ = VK_NULL_HANDLE;
        }

        if (imageAvailableSemaphore_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);
            imageAvailableSemaphore_ = VK_NULL_HANDLE;
        }

        // Command Pool 拥有从它分配出来的 Command Buffer。
        // 销毁命令池会一起释放这些命令缓冲，避免它们继续引用后续要销毁的 Framebuffer 和 Pipeline。
        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, commandPool_, nullptr);
            commandPool_ = VK_NULL_HANDLE;
            commandBuffers_.clear();
        }

        if (indexBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, indexBuffer_, nullptr);
            indexBuffer_ = VK_NULL_HANDLE;
        }

        if (indexBufferMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, indexBufferMemory_, nullptr);
            indexBufferMemory_ = VK_NULL_HANDLE;
        }

        if (descriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
            descriptorSet_ = VK_NULL_HANDLE;
        }

        if (uniformBufferMapped_ != nullptr) {
            vkUnmapMemory(device_, uniformBufferMemory_);
            uniformBufferMapped_ = nullptr;
        }

        if (uniformBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, uniformBuffer_, nullptr);
            uniformBuffer_ = VK_NULL_HANDLE;
        }

        if (uniformBufferMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, uniformBufferMemory_, nullptr);
            uniformBufferMemory_ = VK_NULL_HANDLE;
        }

        // Vertex Buffer 使用单独绑定的 VkDeviceMemory。
        // 先销毁 Buffer 句柄，再释放内存；此时设备已经空闲，命令缓冲也已经释放，不会再引用它。
        if (vertexBuffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, vertexBuffer_, nullptr);
            vertexBuffer_ = VK_NULL_HANDLE;
        }

        if (vertexBufferMemory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, vertexBufferMemory_, nullptr);
            vertexBufferMemory_ = VK_NULL_HANDLE;
        }

        // Framebuffer 绑定了 Swapchain Image View 和 Render Pass，要在它们之前销毁。
        for (VkFramebuffer framebuffer : swapChainFramebuffers_) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
        swapChainFramebuffers_.clear();

        // 图形管线和 Pipeline Layout 都是设备级对象，要在 Render Pass 和 VkDevice 销毁前释放。
        if (graphicsPipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
            graphicsPipeline_ = VK_NULL_HANDLE;
        }

        if (pipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
            pipelineLayout_ = VK_NULL_HANDLE;
        }

        if (descriptorSetLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
            descriptorSetLayout_ = VK_NULL_HANDLE;
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
