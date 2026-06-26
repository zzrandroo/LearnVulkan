# 10：顶点缓冲和三角形

## 学习目标

本阶段的目标是定义顶点结构，创建 Vertex Buffer，把三角形顶点位置和颜色上传到 Vulkan Buffer，并在命令缓冲中绑定这个顶点缓冲进行绘制。

完成本阶段后，窗口中仍然显示三角形，但顶点数据不再写死在 shader 中，而是来自 CPU 侧的 `vertices` 数组和 Vulkan `VkBuffer`。

## 关键概念

- Vertex Buffer：保存顶点属性数据的 Buffer，例如位置、颜色、法线、纹理坐标等。
- `VkBuffer`：Vulkan 中通用的线性数据资源。本阶段用它承载顶点数据。
- `VkDeviceMemory`：Buffer 背后的实际内存。创建 Buffer 后，需要查询内存需求、分配内存并绑定。
- Host Visible：CPU 可以映射并写入的内存类型，适合本阶段直接上传少量顶点数据。
- Host Coherent：CPU 写入后不需要手动 flush，代码更简单。
- Buffer Usage：说明 Buffer 将被如何使用。本阶段使用 `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`。
- 顶点输入布局：告诉图形管线如何从每个 `Vertex` 中读取 `position` 和 `color`，并对应到 shader 的 `layout(location = 0/1)`。

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
build\archives\10-vertex-buffer-triangle\Debug\LearnVulkanStage10.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage10.exe
```

运行后会显示一个 `Learn Vulkan - Vertex Buffer Triangle` 窗口，并在控制台输出类似内容：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Selected GPU: <你的 GPU 名称>
Swapchain: created
Graphics pipeline: created
Vertex buffer: created with 3 vertices
Command buffers recorded: 3
Sync objects: created
```

窗口中应能看到带有顶点颜色插值的三角形。

## 代码说明

核心代码在 `src/main.cpp`：

- `Vertex` 定义顶点结构，包含 `position` 和 `color`。
- `Vertex::getBindingDescription()` 描述每个顶点的步长和输入频率。
- `Vertex::getAttributeDescriptions()` 把 `position` 映射到 shader location 0，把 `color` 映射到 shader location 1。
- `findMemoryType()` 根据 Buffer 的内存需求和目标属性选择合适的内存类型。
- `createBuffer()` 创建 `VkBuffer`、分配 `VkDeviceMemory` 并绑定。
- `createVertexBuffer()` 创建 Host Visible + Host Coherent 的顶点缓冲，映射内存并拷贝 `vertices` 数据。
- `createGraphicsPipeline()` 启用真实顶点输入布局。
- `createCommandBuffers()` 使用 `vkCmdBindVertexBuffers` 绑定顶点缓冲，再调用 `vkCmdDraw`。

中文注释重点解释了为什么本阶段选择 Host Visible 内存、Buffer 和 Device Memory 的关系，以及顶点结构如何对应 shader 输入。

## 常见问题

- 三角形不显示或形状异常：检查 `Vertex` 结构、attribute offset、shader location 是否一致。
- 颜色不对：检查 `VK_FORMAT_R32G32B32_SFLOAT` 是否对应三分量 `float color[3]`。
- `Failed to find suitable memory type.`：当前设备没有满足指定属性的内存类型。对普通桌面 Vulkan 设备，Host Visible + Host Coherent 通常可用。
- `Failed to map vertex buffer memory.`：确认分配的内存类型包含 `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`。
- 性能不是最优：这是预期的学习取舍。本阶段直接使用 CPU 可见内存，下一阶段会学习 Staging Buffer，把最终顶点数据放到更适合 GPU 访问的 Device Local 内存。
- Rider 没有显示 `LearnVulkanStage10`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/10-vertex-buffer-triangle)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Vertex buffers、Vertex input description、Memory requirements 相关章节，以及 Vulkan 规范中 `vkCreateBuffer`、`vkGetBufferMemoryRequirements`、`vkAllocateMemory`、`vkBindBufferMemory`、`vkCmdBindVertexBuffers` 的接口说明。
- 我的理解：Vertex Buffer 是把“顶点数据”从 shader 代码里解放出来的第一步。Buffer 描述资源形状，Device Memory 提供实际存储，顶点输入布局则把内存中的字节解释成 shader 可读的属性。

## 下一步

下一阶段进入 `11-staging-index-buffer`，目标是使用 Staging Buffer 上传数据，创建 Index Buffer，并使用 `vkCmdDrawIndexed` 绘制。
