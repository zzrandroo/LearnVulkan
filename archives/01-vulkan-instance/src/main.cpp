#include <vulkan/vulkan.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void check(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(message) + " VkResult=" + std::to_string(result));
    }
}

VkInstance createInstance() {
    // VkApplicationInfo 用来告诉驱动这个程序和引擎的基本信息。
    // 这些信息不直接决定功能，但有助于驱动和调试工具识别应用。
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Learn Vulkan Stage 01";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // VkInstance 是 Vulkan 程序最外层的上下文对象。
    // 后续创建调试回调、Surface、物理设备枚举等操作都需要依赖它。
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkInstance instance = VK_NULL_HANDLE;
    check(vkCreateInstance(&createInfo, nullptr, &instance), "Failed to create Vulkan instance.");
    return instance;
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
        // 先创建 VkInstance，再通过它查询当前系统中支持 Vulkan 的物理设备。
        const VkInstance instance = createInstance();
        const auto devices = enumeratePhysicalDevices(instance);

        std::cout << "Hello, ,Vulkan!" << '\n';
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

        // VkInstance 必须在所有依赖它的 Vulkan 对象销毁之后再销毁。
        // 当前阶段还没有创建其他 Vulkan 对象，所以这里可以直接释放实例。
        vkDestroyInstance(instance, nullptr);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
