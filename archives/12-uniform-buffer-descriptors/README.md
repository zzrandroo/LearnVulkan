# 12：Uniform Buffer 和描述符

## 学习目标

本阶段的目标是创建 Uniform Buffer、Descriptor Set Layout、Descriptor Pool 和 Descriptor Set，并在每帧更新 MVP 矩阵，让三角形随时间旋转。

完成本阶段后，Vertex Buffer 和 Index Buffer 仍然负责顶点数据；Uniform Buffer 则负责把每帧变化的矩阵传给 shader。

## 关键概念

- Uniform Buffer：保存 shader 读取的小块统一数据，本阶段保存一个 `mat4 mvp`。
- Descriptor Set Layout：声明 shader 会通过哪些 binding 访问资源。
- Descriptor Pool：Descriptor Set 的分配池。
- Descriptor Set：把具体 Buffer 绑定到 shader 看到的 binding 上。
- Pipeline Layout：连接图形管线和 Descriptor Set Layout，让 shader 资源接口成为管线的一部分。
- MVP 矩阵：本阶段用一个 Z 轴旋转矩阵演示 CPU 每帧更新 Uniform Buffer。

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

## 运行方式

从项目根目录构建后，Debug 可执行文件通常位于：

```text
build\archives\12-uniform-buffer-descriptors\Debug\LearnVulkanStage12.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage12.exe
```

运行后会显示一个 `Learn Vulkan - Uniform Buffer Descriptors` 窗口。窗口中的三角形应持续旋转，控制台会输出 Uniform Buffer 和 Descriptor Set 创建信息。

## 代码说明

核心代码在 `src/main.cpp`：

- `UniformBufferObject` 保存传给 shader 的 `mvp` 矩阵。
- `makeUniformBufferObject()` 根据运行时间生成 Z 轴旋转矩阵。
- `createDescriptorSetLayout()` 声明 binding 0 是 vertex shader 可见的 Uniform Buffer。
- `createUniformBuffer()` 创建并常驻映射 Uniform Buffer。
- `createDescriptorPool()` 和 `createDescriptorSet()` 分配并更新 Descriptor Set。
- `createGraphicsPipeline()` 把 Descriptor Set Layout 接入 Pipeline Layout。
- `createCommandBuffers()` 使用 `vkCmdBindDescriptorSets` 绑定 Descriptor Set。
- `drawFrame()` 每帧调用 `updateUniformBuffer()`，再提交绘制命令。

中文注释重点解释了 Descriptor Set 如何把 CPU 侧资源绑定到 shader，以及为什么小型、每帧更新的 Uniform Buffer 可以使用 Host Visible 内存。

## 常见问题

- 三角形不旋转：检查 `drawFrame()` 是否每帧调用 `updateUniformBuffer()`，以及 shader 是否使用 `ubo.mvp`。
- `Failed to allocate descriptor set.`：检查 Descriptor Pool 的 `maxSets` 和 `descriptorCount` 是否足够。
- 验证层提示 descriptor 未绑定：确认命令缓冲中已经调用 `vkCmdBindDescriptorSets`，并且 Pipeline Layout 使用了同一个 Descriptor Set Layout。
- 矩阵结果异常：注意 GLSL 默认按列主序读取 `mat4`，C++ 侧数组按列主序填写。
- Rider 没有显示 `LearnVulkanStage12`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/12-uniform-buffer-descriptors)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Uniform buffers、Descriptor layout and buffer、Descriptor pool and sets 章节，以及 Vulkan 规范中 `vkCreateDescriptorSetLayout`、`vkUpdateDescriptorSets`、`vkCmdBindDescriptorSets` 的接口说明。
- 我的理解：Buffer 只是资源本身，Descriptor Set 才是 shader 访问资源的绑定表。Pipeline Layout 把这张绑定表的结构固定下来，Command Buffer 再绑定具体的 Descriptor Set，让 shader 能读到当前帧的矩阵。

## 下一步

下一阶段进入 `13-texture`，目标是加载图片，创建 Vulkan Image、Image View 和 Sampler，并在 shader 中采样纹理。
