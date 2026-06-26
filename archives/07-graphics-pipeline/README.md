# 07：图形管线

## 学习目标

本阶段的目标是编写最小顶点着色器和片元着色器，编译为 SPIR-V，并创建 `VkPipelineLayout` 和 `VkPipeline`。

完成本阶段后，程序能成功创建图形管线，但窗口仍然不会显示三角形。原因是还没有 Framebuffer、Command Buffer 和提交队列的渲染循环，这些内容会在后续阶段继续加入。

## 关键概念

- Shader：运行在 GPU 上的小程序。顶点着色器负责输出裁剪空间坐标，片元着色器负责输出颜色。
- SPIR-V：Vulkan 使用的着色器二进制格式。GLSL 源码需要先通过 `glslc` 编译成 `.spv`。
- `VkShaderModule`：驱动可识别的 SPIR-V 着色器对象，只在创建管线时需要。
- 固定功能管线：顶点输入、输入装配、视口、裁剪、光栅化、多重采样、颜色混合等 shader 之外的管线状态。
- `VkPipelineLayout`：描述 shader 会访问哪些 Descriptor Set 和 Push Constant。本阶段 shader 没有外部资源，所以使用空布局。
- `VkPipeline`：把 shader、固定功能状态、Pipeline Layout 和 Render Pass 绑定在一起的图形管线对象。

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

MSVC 目标已经加入 `/utf-8`，确保中文注释按 UTF-8 读取。

## Shader 编译

本阶段 shader 源码位于：

```text
shaders\triangle.vert
shaders\triangle.frag
```

使用 Vulkan SDK 提供的 `glslc` 编译：

```powershell
glslc shaders\triangle.vert -o shaders\triangle.vert.spv
glslc shaders\triangle.frag -o shaders\triangle.frag.spv
```

顶点着色器直接使用 `gl_VertexIndex` 生成三角形坐标，因此本阶段不需要 Vertex Buffer。这样可以先专注学习图形管线，顶点缓冲会在第十阶段单独展开。

## 运行方式

从项目根目录构建后，Debug 可执行文件通常位于：

```text
build\archives\07-graphics-pipeline\Debug\LearnVulkanStage07.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage07.exe
```

运行后会显示一个 `Learn Vulkan - Graphics Pipeline` 窗口，并在控制台输出类似内容：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Found 1 Vulkan device(s).
Selected GPU: <你的 GPU 名称>
Logical device: created
Swapchain: created
Swapchain image view count: 3
Render pass: created
Graphics pipeline: created
```

## 代码说明

核心代码在 `src/main.cpp`：

- `readFile()` 以二进制方式读取 `.spv` 文件。
- `createShaderModule()` 把 SPIR-V 字节码包装成 `VkShaderModule`。
- `createGraphicsPipeline()` 配置 shader stages、空顶点输入、三角形列表、viewport、scissor、rasterizer、multisampling、color blending、Pipeline Layout 和 Graphics Pipeline。
- `SHADER_DIR` 由本阶段 CMake 传入，避免运行目录不同导致程序找不到 `.spv` 文件。
- `cleanup()` 先销毁 `graphicsPipeline_` 和 `pipelineLayout_`，再销毁 Render Pass、Image View、Swapchain 和逻辑设备。

中文注释重点解释了为什么本阶段不使用 Vertex Buffer、Pipeline Layout 当前为什么为空，以及各个固定功能管线状态分别负责什么。

## 常见问题

- `Failed to open file: ...triangle.vert.spv`：先确认已经运行 shader 编译命令，并且 `.spv` 文件存在于 `shaders` 目录。
- `Failed to create shader module.`：通常是 `.spv` 文件为空、损坏，或不是用 Vulkan 兼容方式编译出来的 SPIR-V。
- `Failed to create graphics pipeline.`：优先检查 shader stage、Render Pass、Pipeline Layout 和颜色附件格式是否匹配。
- 窗口仍然为空白：这是预期行为。本阶段只创建图形管线，还没有 Framebuffer 和 Command Buffer。
- Rider 没有显示 `LearnVulkanStage07`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/07-graphics-pipeline)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Graphics pipeline basics、Shader modules、Fixed functions、Render passes 相关章节，以及 Vulkan 规范中 `vkCreateShaderModule`、`vkCreatePipelineLayout`、`vkCreateGraphicsPipelines` 的接口说明。
- 我的理解：图形管线是 Vulkan 渲染状态的“总装配”。Shader 只负责可编程部分，Vulkan 还要求我们显式声明大量固定功能状态。这样更啰嗦，但也让驱动能提前知道渲染方式，并把状态组合编译成适合 GPU 执行的管线对象。

## 下一步

下一阶段进入 `08-framebuffer-command-buffer`，目标是为每个 Swapchain 图像创建 Framebuffer，创建 Command Pool，并录制包含 Render Pass、绑定管线和绘制命令的 Command Buffer。
