# 08：Framebuffer 和 Command Buffer

## 学习目标

本阶段的目标是为每个 Swapchain 图像创建 `VkFramebuffer`，创建 `VkCommandPool`，并录制 `VkCommandBuffer`。

完成本阶段后，程序已经拥有一组“将来可以提交给 GPU 执行”的绘制命令：开始 Render Pass、绑定图形管线、绘制三角形、结束 Render Pass。但窗口仍然不会显示画面，因为还没有第九阶段的同步对象和 draw frame 提交流程。

## 关键概念

- `VkFramebuffer`：把 Render Pass 中声明的附件绑定到真实的 Image View。本阶段每个 Swapchain Image View 对应一个 Framebuffer。
- `VkCommandPool`：用于分配 Command Buffer，并且绑定到一个队列族。本阶段使用图形队列族。
- `VkCommandBuffer`：记录 GPU 将来要执行的命令。录制命令不会立即执行，只有提交到队列后才会被 GPU 处理。
- `vkCmdBeginRenderPass`：开始一次 Render Pass，并指定当前使用哪个 Framebuffer。
- `vkCmdBindPipeline`：绑定图形管线，让后续绘制命令使用这套 shader 和固定功能状态。
- `vkCmdDraw`：记录绘制命令。本阶段顶点 shader 使用 `gl_VertexIndex` 生成三个顶点，因此直接绘制 3 个顶点。

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
build\archives\08-framebuffer-command-buffer\Debug\LearnVulkanStage08.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage08.exe
```

运行后会显示一个 `Learn Vulkan - Framebuffer Command Buffer` 窗口，并在控制台输出类似内容：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Found 1 Vulkan device(s).
Selected GPU: <你的 GPU 名称>
Logical device: created
Swapchain: created
Render pass: created
Graphics pipeline: created
Framebuffer count: 3
Command buffers recorded: 3
```

## 代码说明

核心代码在 `src/main.cpp`：

- `createFramebuffers()` 为每个 Swapchain Image View 创建一个 `VkFramebuffer`，让 Render Pass 的颜色附件有真实图像可写。
- `createCommandPool()` 在图形队列族上创建命令池，用于分配图形命令缓冲。
- `createCommandBuffers()` 为每个 Framebuffer 录制一份命令：开始 Render Pass、绑定图形管线、调用 `vkCmdDraw`、结束 Render Pass。
- `cleanup()` 先销毁 Command Pool，从而释放 Command Buffer；再销毁 Framebuffer、Pipeline、Render Pass、Image View、Swapchain 和逻辑设备。

中文注释重点解释了 Framebuffer 如何把 Render Pass 附件落到真实图像上，以及 Command Buffer 录制和提交执行之间的区别。

## 常见问题

- `Failed to create framebuffer.`：优先检查 Framebuffer 使用的 Render Pass 是否与创建图形管线时使用的 Render Pass 一致，以及附件数量是否和 Render Pass 声明匹配。
- `Failed to create command pool.`：检查图形队列族索引是否有效，命令池必须绑定到实际支持图形命令的队列族。
- `Failed to record command buffer.`：通常是 Render Pass、Framebuffer、Pipeline 或命令录制顺序配置不正确。优先看验证层输出。
- 窗口仍然为空白：这是预期行为。本阶段只录制命令，还没有 acquire image、submit queue、present image 和同步对象。
- Rider 没有显示 `LearnVulkanStage08`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/08-framebuffer-command-buffer)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Framebuffers、Command pools、Command buffers 章节，以及 Vulkan 规范中 `vkCreateFramebuffer`、`vkCreateCommandPool`、`vkAllocateCommandBuffers`、`vkCmdBeginRenderPass`、`vkCmdDraw` 的接口说明。
- 我的理解：Render Pass 像是“这次渲染需要哪些附件”的描述，Framebuffer 则把这些附件连接到真实图像。Command Buffer 像是一份给 GPU 的任务清单，录制时只是写清单，提交到队列后才真正开始工作。

## 下一步

下一阶段进入 `09-sync-render-loop`，目标是创建 Semaphore 和 Fence，实现基础 draw frame 流程，把本阶段录制好的 Command Buffer 真正提交给图形队列并呈现到窗口。
