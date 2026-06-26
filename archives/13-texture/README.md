# 13：纹理

## 学习目标

本阶段的目标是加载图片，创建 Vulkan `VkImage`、`VkImageView` 和 `VkSampler`，并通过 Descriptor Set 让 fragment shader 采样纹理。

完成本阶段后，三角形会显示来自 `assets/checker.ppm` 的棋盘格纹理，同时继续使用 Uniform Buffer 进行旋转。

## 关键概念

- Texture Image：GPU 侧的图像资源，本阶段使用 `VK_FORMAT_R8G8B8A8_SRGB`。
- Image Layout：Vulkan 图像必须处在匹配用途的布局中。本阶段经历 `UNDEFINED -> TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL`。
- Image View：shader 不能直接采样裸 `VkImage`，需要通过 `VkImageView` 描述如何访问图像。
- Sampler：描述纹理采样方式，例如过滤、寻址模式和坐标规则。
- Combined Image Sampler：Descriptor 中常用的纹理绑定方式，把 Image View 和 Sampler 一起暴露给 shader。
- UV 坐标：顶点属性之一，决定 fragment shader 从纹理的哪个位置采样。

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
build\archives\13-texture\Debug\LearnVulkanStage13.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage13.exe
```

运行后会显示一个 `Learn Vulkan - Texture` 窗口。窗口中的三角形应显示棋盘格纹理，并继续旋转。

## 代码说明

核心代码在 `src/main.cpp`：

- `assets/checker.ppm` 是本阶段自带的 P3 PPM 小纹理。
- `loadPpmTexture()` 读取图片并转换为 RGBA 像素数据。
- `createTextureImage()` 通过 staging buffer 上传像素，创建 device local 的 `VkImage`。
- `transitionImageLayout()` 使用 pipeline barrier 转换图像布局。
- `copyBufferToImage()` 把 staging buffer 中的像素拷贝到纹理 Image。
- `createTextureImageView()` 为纹理 Image 创建采样视图。
- `createTextureSampler()` 创建最近邻采样器，方便观察小纹理格子。
- `createDescriptorSetLayout()` 新增 binding 1，类型为 `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER`。
- `createDescriptorSet()` 同时绑定 Uniform Buffer 和纹理采样器。

中文注释重点解释了 Image Layout 转换和 Sampler 的作用，以及纹理如何通过 Descriptor Set 进入 shader。

## 常见问题

- `Failed to open texture file`：确认 `assets/checker.ppm` 存在，并且 CMake 传入的 `TEXTURE_FILE` 路径正确。
- `Unsupported texture image layout transition.`：本阶段只实现两种必要转换，新增用途时需要补充对应 barrier。
- 验证层提示 image layout 错误：确认 descriptor 中的 `imageLayout` 是 `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`，并且采样前已经完成布局转换。
- 纹理显示倒置或方向不符合预期：检查顶点 UV 坐标。不同图片格式和坐标约定可能方向不同。
- Rider 没有显示 `LearnVulkanStage13`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/13-texture)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Images、Image view and sampler、Combined image sampler 章节，以及 Vulkan 规范中 `vkCreateImage`、`vkCmdCopyBufferToImage`、`vkCreateSampler`、`VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` 的接口说明。
- 我的理解：Buffer 适合线性数据，Image 适合按二维坐标访问的像素数据。纹理采样不只是读一块内存，还需要布局转换、图像视图和采样器共同描述“这张图如何被 shader 看见”。

## 下一步

下一阶段进入 `14-depth-buffer`，目标是创建 Depth Image，配置深度测试，并更新 Render Pass 和 Framebuffer。
