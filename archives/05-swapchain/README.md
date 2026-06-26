# 05：Swapchain

## 学习目标

本阶段的目标是在已有窗口、Surface、逻辑设备和队列的基础上创建 `VkSwapchainKHR`，并获取交换链中的 `VkImage` 句柄。

完成本阶段后，程序仍然不会绘制画面，但已经具备了“向窗口输出图像”的核心容器。后续 Image View、Render Pass、Framebuffer 和 Command Buffer 都会围绕这些交换链图像继续展开。

## 关键概念

- `VK_KHR_swapchain`：创建交换链必须启用的设备扩展。它不是实例扩展，而是逻辑设备扩展。
- `VkSwapchainKHR`：管理一组可呈现到窗口 Surface 的图像。渲染时通常从交换链取出一张图像，绘制完成后再提交呈现。
- Surface 能力：由 `VkSurfaceCapabilitiesKHR` 描述，包括最小/最大图像数量、交换链尺寸范围、当前窗口变换等。
- Surface 格式：由 `VkSurfaceFormatKHR` 描述，包括图像像素格式和颜色空间。本阶段优先选择 `VK_FORMAT_B8G8R8A8_SRGB` 和 `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`。
- 呈现模式：控制图像如何排队显示到屏幕。本阶段优先选择 `VK_PRESENT_MODE_MAILBOX_KHR`，不可用时退回 Vulkan 保证存在的 `VK_PRESENT_MODE_FIFO_KHR`。
- 交换链尺寸：优先使用 Surface 给出的 `currentExtent`；如果平台允许应用选择，则使用 GLFW framebuffer 尺寸并限制在 Surface 支持范围内。

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
build\archives\05-swapchain\Debug\LearnVulkanStage05.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage05.exe
```

运行后会显示一个 `Learn Vulkan - Swapchain` 窗口，并在控制台输出类似内容：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Found 1 Vulkan device(s).
GPU: <你的 GPU 名称> | API 1.x.x | graphics queue: 0 | present queue: 0 | swapchain extension: yes | formats: 2 | present modes: 3
Selected GPU: <你的 GPU 名称>
Logical device: created
Graphics queue family: 0
Present queue family: 0
Swapchain: created
Swapchain image count: 3
Swapchain format: 50
Swapchain extent: 800x600
```

`Swapchain format` 当前直接输出 Vulkan 枚举值。后续阶段关注图像视图和渲染流程时，可以再把这些枚举值封装成更易读的名称。

## 代码说明

核心代码在 `src/main.cpp`：

- `deviceExtensions` 声明本阶段需要的设备扩展 `VK_KHR_swapchain`。
- `checkDeviceExtensionSupport()` 检查物理设备是否支持交换链扩展。
- `querySwapChainSupport()` 查询 Surface 能力、可用格式和可用呈现模式。
- `isDeviceSuitable()` 在第四阶段队列族条件之外，新增交换链扩展和 Surface 配置检查。
- `chooseSwapSurfaceFormat()` 优先选择适合窗口显示的 BGRA sRGB 格式。
- `chooseSwapPresentMode()` 优先选择 `MAILBOX`，不可用时使用规范保证存在的 `FIFO`。
- `chooseSwapExtent()` 选择交换链图像尺寸，并解释为什么高 DPI 环境下要使用 framebuffer 尺寸。
- `createSwapChain()` 创建 `VkSwapchainKHR`，获取交换链图像，并保存图像格式和尺寸。
- `cleanup()` 在销毁逻辑设备前先销毁 Swapchain，保持设备级对象生命周期顺序正确。

中文注释重点解释了 Swapchain 在窗口显示中的角色，以及为什么格式、呈现模式、图像数量和尺寸都必须从 Surface 支持能力中选择。

## 常见问题

- `swapchain extension: no`：当前设备不支持 `VK_KHR_swapchain`，通常说明它不能直接呈现到窗口 Surface。检查显卡驱动、Vulkan SDK 或当前运行环境。
- `formats: 0` 或 `present modes: 0`：即使设备支持扩展，也可能无法为当前 Surface 创建可用交换链。远程桌面、虚拟机或不完整驱动环境更容易出现这个问题。
- `Failed to create swapchain.`：常见原因是图像数量、尺寸、格式或队列共享模式与 Surface 能力不匹配。优先查看验证层输出。
- 图形队列和呈现队列编号不同：本阶段使用 `VK_SHARING_MODE_CONCURRENT` 简化不同队列族之间的图像访问。后续可以学习 `VK_SHARING_MODE_EXCLUSIVE` 和队列族所有权转移。
- 窗口仍然为空白：这是预期行为。本阶段只创建交换链和获取图像，还没有 Image View、Render Pass、Framebuffer 和绘制命令。
- Rider 没有显示 `LearnVulkanStage05`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/05-swapchain)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Swap chain 章节，以及 Vulkan 规范中 `VK_KHR_swapchain`、`vkGetPhysicalDeviceSurfaceCapabilitiesKHR`、`vkCreateSwapchainKHR`、`vkGetSwapchainImagesKHR` 的接口说明。
- 我的理解：Surface 只是“窗口目标”，Swapchain 才是真正承载窗口图像轮换的对象。它把窗口系统、GPU 图像和呈现队列连接起来。创建 Swapchain 之前必须尊重窗口系统给出的限制，因为图像数量、尺寸、格式和呈现节奏都不是应用可以随意决定的。

## 下一步

下一阶段进入 `06-image-view-render-pass`，目标是为每个 Swapchain 图像创建 `VkImageView`，并创建基础 `VkRenderPass`，开始描述渲染输出如何写入交换链图像。
