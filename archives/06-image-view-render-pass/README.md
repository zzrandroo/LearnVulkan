# 06：Image View 和 Render Pass

## 学习目标

本阶段的目标是在已经创建 Swapchain 并获取交换链图像的基础上，为每个 `VkImage` 创建对应的 `VkImageView`，并创建一个最基础的 `VkRenderPass`。

完成本阶段后，程序仍然不会绘制画面，但已经开始描述“渲染输出要写到哪里，以及写入前后图像处于什么状态”。下一阶段创建图形管线时，会依赖这里创建的 Render Pass。

## 关键概念

- `VkImage`：实际图像资源。Swapchain 图像由窗口系统创建和管理，应用只持有句柄，不能单独销毁。
- `VkImageView`：访问图像时使用的视图。它说明图像格式、维度、颜色通道映射，以及要访问哪些 mip 层和数组层。
- Attachment：Render Pass 中声明的渲染附件。本阶段只有一个颜色附件，对应交换链图像。
- `VkAttachmentReference`：把附件列表中的某个附件映射到 Subpass 中的用途，例如颜色输出。
- Subpass：Render Pass 内的一次渲染步骤。本阶段只有一个图形 Subpass，把颜色写入交换链图像。
- 图像布局：Vulkan 用布局描述图像当前适合做什么。本阶段把颜色附件从 `VK_IMAGE_LAYOUT_UNDEFINED` 转换到 `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`，表示渲染结束后可以交给呈现队列显示。

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

本阶段继续使用 GLFW 创建窗口和 Surface。MSVC 目标已经加入 `/utf-8`，确保中文注释按 UTF-8 读取。

## 运行方式

从项目根目录构建后，Debug 可执行文件通常位于：

```text
build\archives\06-image-view-render-pass\Debug\LearnVulkanStage06.exe
```

单独构建本阶段目录时，Debug 可执行文件通常位于：

```text
build\Debug\LearnVulkanStage06.exe
```

运行后会显示一个 `Learn Vulkan - Image View Render Pass` 窗口，并在控制台输出类似内容：

```text
Hello, Vulkan!
Validation layers: enabled
Window surface: created
Found 1 Vulkan device(s).
Selected GPU: <你的 GPU 名称>
Logical device: created
Graphics queue family: 0
Present queue family: 0
Swapchain: created
Swapchain image count: 3
Swapchain image view count: 3
Swapchain format: 50
Swapchain extent: 800x600
Render pass: created
```

当前阶段还没有 Framebuffer 和 Command Buffer，所以窗口内部仍然不会显示 Vulkan 绘制画面。

## 代码说明

核心代码在 `src/main.cpp`：

- `swapChainImageViews_` 保存每张交换链图像对应的 `VkImageView`。
- `renderPass_` 保存本阶段创建的基础 `VkRenderPass`。
- `createImageViews()` 为每个交换链 `VkImage` 创建 2D 颜色视图，使用交换链图像格式，只覆盖第 0 个 mip 层和第 0 个数组层。
- `createRenderPass()` 创建一个只有颜色附件的 Render Pass，声明清屏加载、渲染后保留结果，并把最终布局设置为 `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`。
- `VkSubpassDependency` 建立外部流程和颜色附件输出之间的基础依赖，为后续渲染命令写入颜色附件做准备。
- `cleanup()` 先销毁 Render Pass，再销毁 Image View，随后销毁 Swapchain 和逻辑设备，保持依赖对象的释放顺序清晰。

中文注释重点解释了 Image 和 Image View 的区别、Attachment 与 Subpass 的关系，以及 Render Pass 为什么要提前描述加载、存储和布局转换。

## 常见问题

- `Failed to create swapchain image view.`：常见原因是图像格式、aspectMask 或 subresourceRange 配置不正确。本阶段交换链图像只作为颜色附件使用，所以应选择 `VK_IMAGE_ASPECT_COLOR_BIT`。
- `Failed to create render pass.`：优先检查附件格式是否与交换链格式一致，以及附件引用的索引是否存在。
- 验证层提示布局不匹配：Render Pass 的 `initialLayout`、Subpass 中的 attachment layout、`finalLayout` 必须和图像用途一致。本阶段颜色附件在子通道内使用 `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`，结束后转为 `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR`。
- 窗口仍然为空白：这是预期行为。本阶段只创建 Image View 和 Render Pass，还没有 Framebuffer、Pipeline 和命令录制。
- Rider 没有显示 `LearnVulkanStage06`：确认根目录 `CMakeLists.txt` 中已经包含 `add_subdirectory(archives/06-image-view-render-pass)`，然后执行 CMake Reload。

## 来源和理解

- 参考来源：Khronos Vulkan Tutorial 的 Image views 和 Render passes 章节，以及 Vulkan 规范中 `vkCreateImageView`、`VkAttachmentDescription`、`VkSubpassDescription`、`vkCreateRenderPass` 的接口说明。
- 我的理解：Swapchain 给了我们图像，但图像本身还不是渲染流程能直接使用的“视角”。Image View 负责说明如何看待这些图像，Render Pass 负责说明一次渲染如何使用这些图像。它们把“窗口图像”推进到了“渲染附件”的层次。

## 下一步

下一阶段进入 `07-graphics-pipeline`，目标是编写最小顶点着色器和片元着色器，编译 SPIR-V，并创建 `VkPipelineLayout` 和图形管线。
