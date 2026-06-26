# 15：模型加载

## 学习目标

本阶段的目标是加载简单 `.obj` 模型，把模型顶点和索引上传到 Vulkan Buffer，并使用 `vkCmdDrawIndexed` 渲染模型。

完成本阶段后，渲染数据不再写死在 C++ 数组里，而是来自 `assets/simple_model.obj`。

## 关键概念

- OBJ：简单文本模型格式，本阶段读取 `v`、`vt` 和三角形 `f v/vt`。
- 模型顶点转换：把 OBJ 的位置和 UV 转换为项目中的 `Vertex`。
- 模型索引：把面数据转换成 `uint32_t` index buffer。
- GPU Buffer：加载出的模型数据仍然通过 staging buffer 上传到 device local vertex/index buffer。

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
build\archives\15-model-loading\Debug\LearnVulkanStage15.exe
```

运行后会显示 `Learn Vulkan - Model Loading` 窗口。控制台会输出模型顶点数和索引数。

## 代码说明

- `assets/simple_model.obj` 是本阶段自带的简单模型来源。
- `loadObjModel()` 解析位置、UV 和面索引，并生成 `ModelData`。
- `loadModel()` 在 Vulkan 设备选择前读取模型文件，保证后续 Buffer 创建时已有数据。
- `createVertexBuffer()` 和 `createIndexBuffer()` 上传模型数据到 device local buffer。
- `vkCmdBindIndexBuffer()` 使用 `VK_INDEX_TYPE_UINT32`，匹配模型索引类型。

中文注释重点说明了模型文件数据如何转换为 Vulkan Buffer，以及为什么外部模型最终仍要走 staging upload 流程。

## 常见问题

- `Failed to open model file`：确认 `assets/simple_model.obj` 存在，并且 CMake 传入的 `MODEL_FILE` 路径正确。
- `Only v/vt OBJ faces are supported`：本阶段解析器只支持教学用的 `v/vt` 三角面。
- 模型不显示：检查 OBJ 面索引、UV、index type 和 `vkCmdDrawIndexed` 的 index count。

## 来源和理解

- 模型来源：本阶段自制的 `simple_model.obj`，用于演示最小 OBJ 加载流程。
- 参考来源：Khronos Vulkan Tutorial 的 Loading models 章节，以及 OBJ 文本格式的基础结构。
- 我的理解：模型加载的核心不是文件格式本身，而是把外部顶点/索引整理成渲染管线能理解的 Buffer 数据。Vulkan 不关心 OBJ，它只关心最终绑定的 Vertex Buffer 和 Index Buffer。

## 下一步

下一阶段进入 `16-review-refactor`，目标是整理重复代码、抽取生命周期管理思路，并完成阶段复盘。
