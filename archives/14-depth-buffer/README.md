# 14：深度缓冲

## 学习目标

本阶段的目标是创建 Depth Image，配置深度测试，并更新 Render Pass 和 Framebuffer，让渲染流程同时写入颜色附件和深度附件。

完成本阶段后，当前三角形画面外观变化不大，但管线已经具备 3D 遮挡所需的深度测试能力。

## 关键概念

- Depth Image：保存每个像素深度值的图像资源。
- Depth Format：深度图像格式需要由设备支持，本阶段优先选择 `VK_FORMAT_D32_SFLOAT`。
- Depth Attachment：Render Pass 中用于深度测试的附件。
- Depth Test：片元进入颜色输出前，根据深度值判断是否应该保留。
- `VK_COMPARE_OP_LESS`：深度值更小的片元更靠近相机，因此通过测试。

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
build\archives\14-depth-buffer\Debug\LearnVulkanStage14.exe
```

运行后会显示 `Learn Vulkan - Depth Buffer` 窗口。控制台应输出 `Depth buffer: created`。

## 代码说明

- `findSupportedFormat()` 和 `findDepthFormat()` 选择设备支持的深度格式。
- `createDepthResources()` 创建深度 `VkImage`、分配内存并创建 `VkImageView`。
- `createRenderPass()` 新增深度附件，并让 Subpass 同时使用颜色附件和深度附件。
- `createGraphicsPipeline()` 启用 depth test 和 depth write。
- `createFramebuffers()` 为每个 Framebuffer 同时绑定 Swapchain Image View 和 Depth Image View。

中文注释重点说明了深度格式选择、深度附件生命周期，以及 Render Pass 如何描述深度布局。

## 常见问题

- `Failed to find supported image format.`：当前设备不支持候选深度格式，需扩展候选列表。
- 验证层提示附件数量不匹配：检查 Render Pass 的 attachments 和 Framebuffer 的 attachments 是否一致。
- 深度测试无效：检查 `pDepthStencilState` 是否设置，深度附件 layout 是否为 `VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL`。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Depth buffering 章节，以及 Vulkan 规范中深度附件、格式属性、Pipeline Depth Stencil State 的说明。
- 我的理解：颜色附件决定“看到什么颜色”，深度附件决定“哪个片元更靠前”。Render Pass 和 Pipeline 都必须知道深度附件的存在，深度测试才会真正参与绘制。

## 下一步

下一阶段进入 `15-model-loading`，目标是加载简单 `.obj` 模型，上传模型顶点和索引，并渲染模型。
