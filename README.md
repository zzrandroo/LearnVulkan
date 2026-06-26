# Learn Vulkan

循序渐进学习 Vulkan 的 C++ 项目。当前主线代码进入第三阶段：使用 GLFW 创建窗口，并创建 Vulkan `VkSurfaceKHR`。

## Open in Rider

1. Start Rider.
2. Open `E:\Learn Vulkan`.
3. Let Rider load the CMake project.
4. Build and run the `LearnVulkanHello` target, or run a specific archive target such as `LearnVulkanStage03`.

The project expects the Vulkan SDK environment variable to be available:

```text
VULKAN_SDK=C:\VulkanSDK\1.4.328.1
```

If Rider was already open before installing Vulkan SDK, restart Rider once.

## Current Stage

- Main target: `LearnVulkanHello`
- Latest archive target: `LearnVulkanStage03`
- Latest archive path: `archives/03-window-surface`
