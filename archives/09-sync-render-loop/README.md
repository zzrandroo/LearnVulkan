# 09：同步和渲染循环

## 学习目标

本阶段的目标是创建 `VkSemaphore` 和 `VkFence`，实现最小 `drawFrame()` 流程，把上一阶段录制好的 Command Buffer 真正提交到图形队列，并把渲染结果呈现到窗口。

完成本阶段后，窗口中应该能看到第一张 Vulkan 渲染画面：深色背景上一枚由 shader 生成的三角形。

## 关键概念

- image available semaphore：`vkAcquireNextImageKHR` 获取到可写入的 Swapchain 图像后发出信号，保证 GPU 不会在图像还不可用时开始渲染。
- render finished semaphore：图形队列执行完 Command Buffer 后发出信号，保证呈现队列不会在渲染完成前把图像拿去显示。
- in flight fence：CPU 等待上一帧 GPU 工作完成后再复用同步对象，避免 CPU 过快提交造成资源仍在使用。
- `vkAcquireNextImageKHR`：从 Swapchain 取得下一张可渲染图像的索引。
- `vkQueueSubmit`：把录制好的 Command Buffer 提交给图形队列，GPU 从这里开始真正执行绘制命令。
- `vkQueuePresentKHR`：把渲染完成的 Swapchain 图像提交给呈现队列显示到窗口。

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

如果修改了 shader，需要重新编译 SPIR-V：

```powershell
glslc shaders\triangle.vert -o shaders\triangle.vert.spv
glslc shaders\triangle.frag -o shaders\triangle.frag.spv
```

MSVC 目标已经加入 `/utf-8`，确保中文注释按 UTF-8 读取。

## 运行方式

从项目根目录构建后，Debug 可执行文件通常位于：

```text
build\archives\09-sync-render-loop\Debug\LearnVulkanStage09.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage09.exe
```

运行后会显示一个 `Learn Vulkan - Sync Render Loop` 窗口，并在控制台输出类似内容：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Selected GPU: <你的 GPU 名称>
Swapchain: created
Render pass: created
Graphics pipeline: created
Framebuffer count: 3
Command buffers recorded: 3
Sync objects: created
```

窗口中应能看到深色背景和一个固定颜色的三角形。

## 代码说明

核心代码在 `src/main.cpp`：

- `createSyncObjects()` 创建 `imageAvailableSemaphore_`、`renderFinishedSemaphore_` 和 `inFlightFence_`。
- `drawFrame()` 实现最小渲染流程：等待 fence、获取图像、重置 fence、提交命令缓冲、呈现图像。
- `mainLoop()` 每轮处理窗口事件后调用 `drawFrame()`，退出时调用 `vkDeviceWaitIdle()`。
- `cleanup()` 在销毁 Command Pool、Framebuffer、Pipeline 和 Swapchain 前，先销毁同步对象并等待设备空闲。

中文注释重点解释了 CPU/GPU 同步关系，以及命令录制、队列提交、图像呈现之间的先后顺序。

## 常见问题

- 窗口没有三角形：确认第八阶段的 Command Buffer 已正确录制 `vkCmdDraw`，并检查验证层是否报告管线、Render Pass 或 Framebuffer 问题。
- `Failed to acquire swapchain image.`：可能是窗口 Surface 或 Swapchain 状态异常。窗口缩放和 Swapchain 重建会在后续阶段处理，本阶段先固定窗口尺寸。
- `Failed to submit draw command buffer.`：优先检查 wait semaphore、signal semaphore、Command Buffer 和 fence 是否都有效。
- `Failed to present swapchain image.`：通常与呈现队列、Swapchain 图像状态或窗口系统有关，优先查看验证层输出。
- 程序关闭时卡顿：本阶段退出前会等待设备空闲，这是为了保证 GPU 不再使用即将销毁的 Vulkan 对象。
- Rider 没有显示 `LearnVulkanStage09`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/09-sync-render-loop)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Rendering and presentation、Semaphores、Fences 章节，以及 Vulkan 规范中 `vkAcquireNextImageKHR`、`vkQueueSubmit`、`vkQueuePresentKHR`、`vkCreateSemaphore`、`vkCreateFence` 的接口说明。
- 我的理解：前面阶段只是把渲染需要的对象和命令都准备好，第九阶段才把它们串成真正的一帧。Semaphore 处理 GPU 队列之间的顺序，Fence 处理 CPU 和 GPU 之间的节奏；没有这些同步，渲染和呈现就可能抢同一张图像。

## 下一步

下一阶段进入 `10-vertex-buffer-triangle`，目标是定义顶点结构，创建 Vertex Buffer，并上传三角形顶点数据，让三角形顶点不再写死在 shader 中。
