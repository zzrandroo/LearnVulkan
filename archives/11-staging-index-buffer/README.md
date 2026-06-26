# 11：Staging Buffer 和索引缓冲

## 学习目标

本阶段的目标是使用 Staging Buffer 上传顶点和索引数据，把真正绘制时使用的 Vertex Buffer 和 Index Buffer 放到 `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` 内存中，并使用 `vkCmdDrawIndexed` 绘制。

完成本阶段后，窗口仍然显示三角形，但数据上传路径更接近真实 Vulkan 程序：CPU 先写入临时 staging buffer，再通过拷贝命令把数据送到 GPU 更适合读取的缓冲中。

## 关键概念

- Staging Buffer：CPU 可见的临时缓冲，用来上传数据，不直接参与绘制。
- Device Local：更适合 GPU 访问的内存类型，通常不能直接被 CPU 映射。
- Transfer Src / Transfer Dst：Buffer Usage 标记，说明一个缓冲可以作为拷贝源或拷贝目标。
- Index Buffer：保存顶点索引，让绘制命令按索引读取顶点数据。
- `vkCmdCopyBuffer`：在命令缓冲中记录 Buffer 到 Buffer 的拷贝。
- `vkCmdDrawIndexed`：使用索引缓冲绘制，是后续模型和网格渲染的常用方式。

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
build\archives\11-staging-index-buffer\Debug\LearnVulkanStage11.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage11.exe
```

运行后会显示一个 `Learn Vulkan - Staging Index Buffer` 窗口，并在控制台看到 Vertex Buffer、Index Buffer 和 Command Buffer 的创建信息。

## 代码说明

核心代码在 `src/main.cpp`：

- `indices` 保存三角形索引，本阶段使用 `uint16_t`。
- `beginSingleTimeCommands()` 和 `endSingleTimeCommands()` 用于录制并提交一次性拷贝命令。
- `copyBuffer()` 使用 `vkCmdCopyBuffer` 从 staging buffer 拷贝到 device local buffer。
- `createVertexBuffer()` 创建 CPU 可见 staging buffer，写入顶点后拷贝到 device local vertex buffer。
- `createIndexBuffer()` 用同样方式上传索引数据。
- `createCommandBuffers()` 绑定 Vertex Buffer 和 Index Buffer，并调用 `vkCmdDrawIndexed`。

中文注释重点解释了为什么不直接把所有数据放在 Host Visible 内存里，以及 staging buffer、device local buffer、index buffer 的职责差异。

## 常见问题

- `Failed to submit single time command buffer.`：检查 Command Pool 是否已经创建，图形队列是否有效。
- `Failed to find suitable memory type.`：确认请求的内存属性与当前设备支持的内存类型匹配。
- 三角形不显示：检查 `indices` 是否引用了有效顶点，以及 `vkCmdBindIndexBuffer` 的索引类型是否为 `VK_INDEX_TYPE_UINT16`。
- 数据上传后颜色异常：检查 Vertex 结构和 shader location 是否仍然一致。
- Rider 没有显示 `LearnVulkanStage11`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/11-staging-index-buffer)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Staging buffer、Index buffer 章节，以及 Vulkan 规范中 `vkCmdCopyBuffer`、`vkCmdBindIndexBuffer`、`vkCmdDrawIndexed` 的接口说明。
- 我的理解：Host Visible 内存方便 CPU 写入，但不一定适合 GPU 高频读取。Staging Buffer 把“上传”变成一次明确的传输步骤，最终绘制数据放在 Device Local 内存中，这是 Vulkan 常见的资源上传模式。

## 下一步

下一阶段进入 `12-uniform-buffer-descriptors`，目标是创建 Uniform Buffer、Descriptor Set Layout、Descriptor Pool 和 Descriptor Set，并每帧更新 MVP 矩阵。
