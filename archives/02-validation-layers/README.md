# 02：验证层和调试回调

## 学习目标

本阶段的目标是在第一阶段 `VkInstance` 和物理设备枚举的基础上，启用 Vulkan Validation Layers，并创建 `VkDebugUtilsMessengerEXT`，让 Vulkan API 使用错误能以可读的调试信息输出到控制台。

## 关键概念

- Validation Layers：Vulkan 的诊断层，用来检查参数、对象生命周期、同步和常见误用。它不是 Vulkan 运行必需功能，通常只在 Debug 构建中启用。
- `VK_LAYER_KHRONOS_validation`：Khronos 维护的标准验证层，开发阶段最常用。
- `VK_EXT_debug_utils`：提供 `VkDebugUtilsMessengerEXT` 的实例扩展，用来把验证层消息回调到应用程序。
- `vkGetInstanceProcAddr`：扩展函数需要通过实例查询函数指针，因为它们不一定由核心 Vulkan 版本直接提供。
- 生命周期顺序：先创建 `VkInstance`，再创建 `VkDebugUtilsMessengerEXT`；退出时先销毁调试回调，再销毁实例。

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

MSVC 目标已加入 `/utf-8`，确保中文注释按 UTF-8 读取。

## 运行方式

从项目根目录构建后，Debug 可执行文件通常位于：

```text
build\archives\02-validation-layers\Debug\LearnVulkanStage02.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage02.exe
```

运行后期望看到类似输出：

```text
Hello, Vulkan!
Validation layers: enabled
Found 1 Vulkan device(s).
GPU 0: <你的 GPU 名称> | API 1.x.x
```

如果触发验证层消息，会看到类似：

```text
[Vulkan validation] <验证层输出的消息>
```

本阶段为了确认回调已经接通，代码同时监听了 `VERBOSE`、`WARNING` 和 `ERROR` 级别。即使程序没有写错 Vulkan 调用，Debug 运行时也可能看到 loader 加载 ICD 或验证层库的详细日志。

## 代码说明

核心代码在 `src/main.cpp`：

- `checkValidationLayerSupport()` 枚举实例层，确认系统中存在 `VK_LAYER_KHRONOS_validation`。
- `getRequiredExtensions()` 在 Debug 构建中加入 `VK_EXT_debug_utils`。
- `populateDebugMessengerCreateInfo()` 集中配置消息严重程度、消息类型和回调函数。
- `createDebugUtilsMessengerEXT()` 和 `destroyDebugUtilsMessengerEXT()` 演示如何通过 `vkGetInstanceProcAddr` 加载扩展函数。
- `setupDebugMessenger()` 在实例创建后创建正式的调试回调对象。

代码中的中文注释重点解释验证层的作用、扩展函数加载方式，以及调试回调与 `VkInstance` 的生命周期关系。

## 常见问题

- `VK_LAYER_KHRONOS_validation is not available.`：通常是 Vulkan SDK 没安装完整，或运行环境没有找到 SDK 中的验证层配置。
- `VK_EXT_debug_utils` 创建失败：确认实例创建时已经把 `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` 加入扩展列表。
- Release 构建没有验证层输出：这是预期行为，代码通过 `NDEBUG` 在 Release 中关闭验证层。
- 看不到验证层消息：先确认当前是 Debug 构建，并且输出中显示 `Validation layers: enabled`。不同驱动和 SDK 版本的 verbose 输出不完全一样；真正的错误和警告会在后续阶段更容易出现。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Validation layers 章节，以及 Vulkan SDK 头文件中 `VK_EXT_debug_utils` 相关类型定义。
- 我的理解：验证层像开发期的安全网，它不会替代正确的 Vulkan 生命周期管理，但能在错误刚发生时给出更接近原因的提示。调试回调本身也是一个依赖 `VkInstance` 的 Vulkan 对象，因此它必须参与创建和销毁顺序的管理。

## 下一步

下一阶段进入 `03-window-surface`，目标是引入 GLFW，创建窗口，并通过窗口系统扩展创建 `VkSurfaceKHR`。
