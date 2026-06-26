# 04：队列族和逻辑设备

## 学习目标

本阶段的目标是在已经创建 `VkInstance` 和 `VkSurfaceKHR` 的基础上，选择一个同时支持图形命令和窗口呈现的物理设备，并从它创建 `VkDevice`。

完成本阶段后，程序还不会真正绘制画面，但已经拿到了后续渲染循环必需的两个队列句柄：图形队列和呈现队列。

## 关键概念

- `VkPhysicalDevice`：系统中的真实 Vulkan 设备，通常对应一块 GPU。它只用于查询能力，不需要手动销毁。
- 队列族：一组能力相同的队列。不同队列族可能分别支持图形、计算、传输或呈现。
- 图形队列：支持 `VK_QUEUE_GRAPHICS_BIT` 的队列，后续会接收绘制命令。
- 呈现队列：支持把图像呈现到当前 `VkSurfaceKHR` 的队列。呈现能力依赖具体窗口 Surface，因此必须用 `vkGetPhysicalDeviceSurfaceSupportKHR` 查询。
- `VkDevice`：从物理设备创建的逻辑设备。后续 Swapchain、Buffer、Image、Pipeline 等大多数 Vulkan 对象都会依赖它。
- `VkQueue`：从逻辑设备取出的队列句柄。队列不单独销毁，它的生命周期跟随 `VkDevice`。

## 构建方式

在项目根目录构建全部阶段：

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

也可以在本阶段目录中单独构建：

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

本阶段继续使用 GLFW 创建窗口和 Surface。MSVC 目标已经加入 `/utf-8`，确保中文注释按 UTF-8 读取。

## 运行方式

从项目根目录构建后，Debug 可执行文件通常位于：

```text
build\archives\04-logical-device-queues\Debug\LearnVulkanStage04.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage04.exe
```

运行后会显示一个 `Learn Vulkan - Logical Device Queues` 窗口，并在控制台输出类似内容：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Found 1 Vulkan device(s).
GPU: <你的 GPU 名称> | API 1.x.x | graphics queue: 0 | present queue: 0
Selected GPU: <你的 GPU 名称>
Logical device: created
Graphics queue family: 0
Present queue family: 0
```

当前阶段还没有 Swapchain 和渲染命令，所以窗口内部仍然不会显示 Vulkan 绘制画面。

## 代码说明

核心代码在 `src/main.cpp`：

- `QueueFamilyIndices` 保存图形队列族和呈现队列族索引，用 `std::optional` 表示是否找到。
- `findQueueFamilies()` 查询每个队列族的能力，并用当前 `VkSurfaceKHR` 检查呈现支持。
- `pickPhysicalDevice()` 枚举所有物理设备，选择第一个同时具备图形和呈现队列族的设备。
- `createLogicalDevice()` 基于选中的队列族创建 `VkDevice`，并通过 `vkGetDeviceQueue` 获取图形队列和呈现队列。
- `cleanup()` 先销毁 `VkDevice`，再销毁 Surface、调试回调、实例和窗口，保持 Vulkan 对象生命周期顺序清晰。

中文注释重点解释了物理设备、逻辑设备、队列族、队列之间的关系，以及为什么呈现能力必须结合具体 Surface 查询。

## 常见问题

- `Failed to find a GPU with graphics and present queue support.`：当前环境没有找到既能执行图形命令又能呈现到窗口 Surface 的 Vulkan 设备。检查显卡驱动、Vulkan SDK、远程桌面环境或虚拟机图形能力。
- `Failed to query queue present support.`：通常与 Surface 创建或平台窗口系统支持有关，优先确认第三阶段窗口和 Surface 能正常创建。
- 控制台中图形队列和呈现队列编号不同：这是正常情况。某些设备会用不同队列族分别处理绘制和呈现，后续创建 Swapchain 时要继续考虑这种差异。
- 窗口为空白：这是预期行为。本阶段只创建逻辑设备和队列，还没有交换链、Render Pass、Framebuffer 和 Command Buffer。
- Rider 没有显示 `LearnVulkanStage04`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/04-logical-device-queues)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Physical devices and queue families、Logical device and queues 章节，以及 Vulkan 规范中 `vkGetPhysicalDeviceQueueFamilyProperties`、`vkGetPhysicalDeviceSurfaceSupportKHR`、`vkCreateDevice` 的接口说明。
- 我的理解：物理设备回答“这块 GPU 能做什么”，逻辑设备回答“这个程序准备使用其中哪些能力”。队列族是两者之间的重要筛选条件，因为 Vulkan 的命令最终都要提交到队列执行；如果没有提前申请正确队列，后面即使创建了 Buffer、Pipeline 或 Swapchain，也没有合适的通道去执行绘制和呈现。

## 下一步

下一阶段进入 `05-swapchain`，目标是查询 Surface 能力，选择 Surface 格式、呈现模式和交换链尺寸，然后创建 `VkSwapchainKHR` 并获取交换链图像。
