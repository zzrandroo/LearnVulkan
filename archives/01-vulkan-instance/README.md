# 01：Vulkan 实例和物理设备

## 学习目标

本阶段的目标是建立一个最小 Vulkan 程序：创建 `VkInstance`，枚举当前系统中的 `VkPhysicalDevice`，并输出 GPU 名称和 Vulkan API 版本。

## 关键概念

- `VkInstance`：Vulkan 应用的顶层上下文，后续很多 Vulkan 对象都要从它开始创建或查询。
- `VkApplicationInfo`：向驱动和调试工具描述应用程序、引擎和目标 Vulkan API 版本。
- `VkPhysicalDevice`：系统中的物理 GPU 或 Vulkan 设备能力入口。
- 枚举模式：先查询数量，再分配数组，最后获取具体对象。

## 构建方式

在本阶段目录中单独配置和构建：

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

如果系统没有自动找到 Vulkan SDK，请先确认环境变量类似：

```text
VULKAN_SDK=C:\VulkanSDK\1.4.328.1
```

## 运行方式

在 Windows + Visual Studio 生成器下，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage01.exe
```

运行后期望看到类似输出：

```text
Hello, Vulkan!
Found 1 Vulkan device(s).
GPU 0: <你的 GPU 名称> | API 1.x.x
```

## 代码说明

核心代码在 `src/main.cpp`：

- `createInstance()` 创建 Vulkan 实例。
- `enumeratePhysicalDevices()` 枚举系统中的 Vulkan 物理设备。
- `main()` 输出设备数量、设备名称和 Vulkan API 版本。

代码中的中文注释重点解释 Vulkan 对象的用途、生命周期和常见枚举流程。

## 常见问题

- `Vulkan::Vulkan` 找不到：通常是 Vulkan SDK 没安装，或安装后 IDE/终端没有重新加载环境变量。
- `Found 0 Vulkan device(s).`：可能是显卡驱动不支持 Vulkan，或当前环境没有可用 Vulkan 设备。
- 程序创建实例失败：先确认 Vulkan SDK、显卡驱动和运行环境是否完整。

## 下一步

下一阶段进入 `02-validation-layers`，目标是启用 Vulkan Validation Layers，并接入调试回调，让错误信息更容易理解。
