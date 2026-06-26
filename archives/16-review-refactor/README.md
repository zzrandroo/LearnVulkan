# 16：项目整理和复盘

## 学习目标

本阶段的目标是保留第十五阶段的完整可运行代码，并对前面阶段的 Vulkan 对象生命周期、模块边界和后续重构方向做一次整理。

这个阶段不新增 Vulkan 功能，重点是让项目能被重新阅读、重新构建，并为后续继续学习打基础。

## 关键概念

- 生命周期分层：Instance、Surface、Device、Swapchain、Render Pass、Pipeline、Buffer、Image、Descriptor、Command Buffer、Sync Object。
- 资源依赖顺序：创建时从底层上下文到具体资源，销毁时反向释放。
- 数据上传路径：CPU 数据先进入 staging buffer，再拷贝到 device local buffer 或 image。
- 渲染帧流程：Acquire Image、Update Uniform、Submit Command Buffer、Present Image。
- 后续重构方向：把单文件示例拆成小型 RAII 类型和清晰模块。

## 构建方式

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

修改 shader 后重新编译：

```powershell
glslc shaders\triangle.vert -o shaders\triangle.vert.spv
glslc shaders\triangle.frag -o shaders\triangle.frag.spv
```

## 运行方式

根目录构建后的可执行文件通常位于：

```text
build\archives\16-review-refactor\Debug\LearnVulkanStage16.exe
```

运行后会显示 `Learn Vulkan - Review Refactor` 窗口，渲染从 OBJ 加载的纹理模型。

## 代码说明

本阶段代码仍位于 `src/main.cpp`，保留完整可运行路径：

- GLFW 窗口和 Vulkan Surface
- 物理设备选择、逻辑设备和队列
- Swapchain、Image View、Render Pass、Depth Buffer、Framebuffer
- Shader、Pipeline Layout、Graphics Pipeline
- Vertex Buffer、Index Buffer、Uniform Buffer
- Texture Image、Texture Image View、Sampler
- Descriptor Set Layout、Descriptor Pool、Descriptor Set
- Command Pool、Command Buffer、Semaphore、Fence
- OBJ 模型加载和 PPM 纹理加载

## 常见问题

- 后续如果代码继续变大，应优先把资源生命周期拆成 RAII 类，避免手动清理顺序越来越难维护。
- Swapchain 重建还没有实现，窗口仍固定不可缩放。
- 当前只有单帧在飞，后续可以扩展为多帧并行。
- OBJ 解析器只支持本阶段教学用的简单 `v/vt` 三角面。

## 来源和理解

- 本阶段复盘基于前 15 个阶段的代码和 README。
- 我的理解：Vulkan 的难点不只是 API 数量，而是对象之间的依赖关系。每一步都要清楚“谁拥有谁”“谁引用谁”“谁必须先销毁”。一旦生命周期顺序清楚，后续 RAII 封装和模块拆分就有了方向。

## 下一步

后续建议：

1. 抽取 `VulkanContext`：管理 Instance、Surface、PhysicalDevice、Device、Queue。
2. 抽取 `SwapchainResources`：管理 Swapchain、Image View、Depth Image、Framebuffer。
3. 抽取 `Buffer` 和 `Image` 小型 RAII 包装。
4. 增加 Swapchain 重建，支持窗口 resize。
5. 增加多帧并行和 per-frame Uniform Buffer。
