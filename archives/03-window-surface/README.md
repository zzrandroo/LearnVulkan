# 03：窗口和 Surface

## 学习目标

本阶段的目标是在验证层和调试回调的基础上引入 GLFW，创建一个桌面窗口，并通过 `glfwCreateWindowSurface` 创建 Vulkan 的 `VkSurfaceKHR`。

`VkSurfaceKHR` 是 Vulkan 和操作系统窗口系统之间的连接点。它还不是 Swapchain，也不会直接显示图像；它只是告诉 Vulkan：后续交换链图像要呈现到哪个平台窗口。

## 关键概念

- GLFW：跨平台窗口库，本阶段用它创建窗口并查询 Vulkan 所需的实例扩展。
- `glfwGetRequiredInstanceExtensions`：返回当前平台创建 Surface 必须启用的 Vulkan 实例扩展，例如 Windows 上通常会包含 Win32 Surface 相关扩展。
- `VkSurfaceKHR`：表示可呈现目标的 Vulkan 对象，依赖 `VkInstance` 和原生窗口句柄。
- `glfwCreateWindowSurface`：GLFW 提供的辅助函数，用来屏蔽 Win32、X11、Wayland 等平台差异。
- 生命周期顺序：先初始化 GLFW 和窗口，再创建 `VkInstance`，再创建 `VkSurfaceKHR`；退出时先销毁 Surface，再销毁调试回调和实例，最后销毁窗口并终止 GLFW。

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

本阶段会优先使用系统已经安装的 `glfw3` CMake 包；如果找不到，会通过 CMake `FetchContent` 下载 GLFW 3.4。MSVC 目标已加入 `/utf-8`，确保中文注释按 UTF-8 读取。

## 运行方式

从项目根目录构建后，Debug 可执行文件通常位于：

```text
build\archives\03-window-surface\Debug\LearnVulkanStage03.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage03.exe
```

运行后期望看到一个 `Learn Vulkan - Window Surface` 窗口，并在控制台看到类似输出：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Found 1 Vulkan device(s).
GPU 0: <你的 GPU 名称> | API 1.x.x
```

当前阶段还没有 Swapchain 和渲染命令，所以窗口内部不会绘制 Vulkan 画面。关闭窗口后程序会正常退出。

## 代码说明

核心代码在 `src/main.cpp`：

- `initWindow()` 初始化 GLFW，关闭 OpenGL 客户端 API，并创建一个只用于 Vulkan 的窗口。
- `getRequiredExtensions()` 从 GLFW 查询创建 Surface 需要的实例扩展，并在 Debug 构建中追加 `VK_EXT_debug_utils`。
- `createInstance()` 创建启用了窗口系统扩展的 `VkInstance`。
- `createSurface()` 调用 `glfwCreateWindowSurface` 创建 `VkSurfaceKHR`。
- `mainLoop()` 只处理窗口事件，让窗口保持到用户关闭。
- `cleanup()` 按 Surface、调试回调、实例、窗口、GLFW 的顺序释放资源。

代码中的中文注释重点解释了为什么要关闭 GLFW 的 OpenGL 上下文、为什么 Surface 扩展必须在实例创建前声明，以及 `VkSurfaceKHR` 与 `VkInstance`、窗口之间的生命周期关系。

## 常见问题

- CMake 下载 GLFW 失败：检查网络，或在系统中提前安装 `glfw3` 并让 CMake 能找到它。
- `GLFW reports that Vulkan is not supported on this system.`：通常是 Vulkan SDK、显卡驱动或运行环境没有正确提供 Vulkan loader。
- `GLFW did not report the required Vulkan instance extensions.`：可能是 GLFW 初始化失败，或当前平台不支持 Vulkan Surface。
- 程序只显示空窗口：这是预期行为。本阶段只创建窗口和 Surface，还没有 Swapchain、Render Pass 和绘制命令。
- 关闭窗口前程序不结束：这是预期行为。`mainLoop()` 会持续轮询窗口事件，直到用户关闭窗口。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Window surface 章节、GLFW 3.4 文档中 Vulkan support 相关接口说明。
- 我的理解：Surface 是进入“把图像显示到窗口”之前必须建立的平台桥梁。Vulkan 本身不负责创建窗口，因此需要 GLFW 这样的库告诉 Vulkan 当前系统要启用哪些实例扩展，并把平台窗口句柄包装成统一的 `VkSurfaceKHR`。

## 下一步

下一阶段进入 `04-logical-device-queues`，目标是基于当前 Surface 查找支持图形操作和窗口呈现的队列族，并创建逻辑设备和队列句柄。
