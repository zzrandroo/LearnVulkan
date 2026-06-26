# Learn Vulkan 项目协作规则

这个文件用于约束本项目后续所有学习、编码、整理和复盘行为。以后在本项目中继续工作时，优先遵守这里的规则。

## 总目标

本项目是一个循序渐进学习 Vulkan 的 C++ 项目。每个阶段只解决一个明确主题，代码要能独立回看、能重新构建、能说明当时学到了什么。

## 固定行为规则

1. 每次新增或修改 Vulkan 示例代码前，先确认当前阶段目标，不把多个阶段的内容混在一起。
2. 每完成一个学习阶段，都要创建一个单独的代码存档，保存当时可运行或可编译的完整代码。
3. 所有新增 C++、CMake、脚本和配置示例中的注释必须使用清晰、不乱码的中文注释。
4. 文件保存时统一使用 UTF-8 编码；如果工具支持，优先使用 UTF-8 without BOM。
5. 中文注释要解释“为什么这样做”和“这个 Vulkan 概念是什么”，不要只重复代码表面含义。
6. 如果代码来自教程、官方文档或外部资料，需要在对应阶段记录来源和自己的理解。
7. 构建目录、临时文件、IDE 生成物不要作为学习代码存档的核心内容。
8. 每个阶段结束时都要记录：阶段目标、关键概念、运行方式、常见错误、下一步计划。
9. 如果使用 MSVC 编译包含中文注释的源码，必须在对应 CMake 目标中加入 `/utf-8`，避免编译器按系统代码页读取 UTF-8 源文件。
10. 每个阶段存档如果需要在 Rider 中调试，必须从根目录 `CMakeLists.txt` 使用 `add_subdirectory(...)` 挂载该阶段目录，让 Rider 能从项目根目录发现对应目标。

## 代码存档规则

每个学习阶段使用独立目录存档，建议结构如下：

```text
archives/
  01-vulkan-instance/
    README.md
    CMakeLists.txt
    src/
      main.cpp
  02-validation-layers/
    README.md
    CMakeLists.txt
    src/
      main.cpp
```

存档目录命名规则：

- 使用两位数字编号，例如 `01`、`02`、`03`。
- 编号后接英文短横线主题名，例如 `vulkan-instance`。
- 不在存档目录中放入 `build/`、`.vs/`、`.idea/` 等生成物。
- 每个存档都要有自己的 `README.md`，说明这一阶段学了什么、怎么构建、怎么运行。

当主线代码进入新阶段时，先复制上一阶段的稳定代码到新的存档目录，再在主线或新阶段目录中继续修改。

## Rider 和 CMake 调试规范

为了避免阶段代码在 Rider 中没有语法高亮、不能设置断点或找不到运行目标，后续每个阶段都按以下方式接入：

1. 阶段目录保留自己的 `CMakeLists.txt`，保证可以单独构建和回看。
2. 根目录 `CMakeLists.txt` 也要通过 `add_subdirectory(archives/阶段目录名)` 挂载该阶段。
3. 阶段目标名必须唯一，例如 `LearnVulkanStage01`、`LearnVulkanStage02`。
4. 在 Rider 中优先打开项目根目录 `E:\Learn Vulkan`，然后通过 CMake Reload 加载所有阶段目标。
5. 如果 Rider 没有语法高亮，优先检查这个文件是否属于已加载的 CMake Target。
6. 如果 Rider 没有显示阶段运行配置，优先检查根 CMake 是否已经挂载该阶段目录。

根目录挂载示例：

```cmake
# 把第一阶段存档也加入根 CMake，方便 Rider 从项目根目录直接加载和调试。
# 存档目录仍然保留自己的 CMakeLists.txt，因此也可以单独打开和构建。
add_subdirectory(archives/01-vulkan-instance)
```

MSVC 编码设置示例：

```cmake
if (MSVC)
    target_compile_options(LearnVulkanStage01 PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(LearnVulkanStage01 PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

## 中文注释规范

新增代码中的中文注释应满足：

- 必须能在 Rider、Visual Studio、VS Code 和 GitHub 中正常显示。
- 注释使用简体中文，避免中英混杂造成理解负担。
- Vulkan 结构体、枚举、函数名保留英文原名。
- 对关键 Vulkan 对象说明生命周期，例如创建、使用、销毁的顺序。
- 对容易踩坑的地方写明原因，例如扩展、验证层、队列族、同步、内存屏障。

示例：

```cpp
// VkApplicationInfo 用来告诉驱动这个程序和引擎的基本信息。
// 这些信息不直接决定功能，但有助于驱动或调试工具识别应用。
VkApplicationInfo appInfo{};
```

不推荐：

```cpp
// 设置 appInfo
VkApplicationInfo appInfo{};
```

## Vulkan 学习计划

### 01. Vulkan 实例和物理设备

目标：

- 创建 `VkInstance`。
- 枚举可用物理设备。
- 输出 GPU 名称和 Vulkan API 版本。

存档目录：

```text
archives/01-vulkan-instance/
```

完成标准：

- 程序能输出 `Hello, Vulkan!`。
- 程序能列出至少一个 Vulkan 设备，或清楚提示当前环境没有可用设备。
- 代码中包含解释 `VkInstance`、`VkApplicationInfo`、物理设备枚举流程的中文注释。

### 02. 验证层和调试回调

目标：

- 启用 Vulkan Validation Layers。
- 创建 `VkDebugUtilsMessengerEXT`。
- 在控制台输出验证层提示。

存档目录：

```text
archives/02-validation-layers/
```

完成标准：

- Debug 构建默认启用验证层。
- Release 构建可以关闭验证层。
- 中文注释说明验证层的作用、扩展函数加载方式和调试回调生命周期。

### 03. 窗口和 Surface

目标：

- 引入窗口库，优先考虑 GLFW。
- 创建应用窗口。
- 创建 Vulkan `VkSurfaceKHR`。

存档目录：

```text
archives/03-window-surface/
```

完成标准：

- 程序显示一个窗口。
- Vulkan 实例包含创建 Surface 所需扩展。
- 退出时按正确顺序销毁 Surface、窗口和 Vulkan 实例。

### 04. 队列族和逻辑设备

目标：

- 查找支持图形操作的队列族。
- 查找支持 Surface 呈现的队列族。
- 创建 `VkDevice` 和队列句柄。

存档目录：

```text
archives/04-logical-device-queues/
```

完成标准：

- 能选择一个满足图形和呈现需求的物理设备。
- 能创建逻辑设备。
- 中文注释说明物理设备、逻辑设备、队列族、队列之间的关系。

### 05. Swapchain

目标：

- 查询 Surface 能力。
- 选择 Surface 格式、呈现模式和交换链尺寸。
- 创建 `VkSwapchainKHR` 并获取交换链图像。

存档目录：

```text
archives/05-swapchain/
```

完成标准：

- 能成功创建 Swapchain。
- 能输出交换链图像数量和格式。
- 中文注释说明 Swapchain 在窗口显示中的角色。

### 06. Image View 和 Render Pass

目标：

- 为 Swapchain 图像创建 `VkImageView`。
- 创建基础 `VkRenderPass`。

存档目录：

```text
archives/06-image-view-render-pass/
```

完成标准：

- 每个 Swapchain 图像都有对应 Image View。
- Render Pass 描述清楚颜色附件的加载、存储和布局转换。
- 中文注释说明 Image、Image View、Attachment、Subpass 的关系。

### 07. 图形管线

目标：

- 编写最小顶点着色器和片元着色器。
- 编译 SPIR-V。
- 创建 `VkPipelineLayout` 和图形管线。

存档目录：

```text
archives/07-graphics-pipeline/
```

完成标准：

- 能成功创建图形管线。
- 着色器源码和管线配置都有中文说明。
- README 记录 shader 编译命令。

### 08. Framebuffer 和 Command Buffer

目标：

- 为每个 Swapchain 图像创建 Framebuffer。
- 创建 Command Pool。
- 录制 Command Buffer。

存档目录：

```text
archives/08-framebuffer-command-buffer/
```

完成标准：

- Command Buffer 中包含开始 Render Pass、绑定管线、绘制、结束 Render Pass。
- 中文注释说明命令录制不是立即执行，而是提交给队列后执行。

### 09. 同步和渲染循环

目标：

- 创建 Semaphore 和 Fence。
- 实现基础 draw frame 流程。
- 处理 CPU 与 GPU 同步。

存档目录：

```text
archives/09-sync-render-loop/
```

完成标准：

- 窗口中能显示第一张 Vulkan 渲染画面。
- 中文注释说明 image available、render finished、in flight fence 的作用。

### 10. 顶点缓冲和三角形

目标：

- 定义顶点结构。
- 创建 Vertex Buffer。
- 上传三角形顶点数据。

存档目录：

```text
archives/10-vertex-buffer-triangle/
```

完成标准：

- 窗口中能渲染一个三角形。
- 中文注释说明 Host Visible、Device Local、Buffer Usage 的选择。

### 11. Staging Buffer 和索引缓冲

目标：

- 使用 Staging Buffer 上传数据。
- 创建 Index Buffer。
- 使用索引绘制。

存档目录：

```text
archives/11-staging-index-buffer/
```

完成标准：

- 使用 `vkCmdDrawIndexed` 绘制。
- 中文注释说明为什么不直接把所有数据放在 Host Visible 内存里。

### 12. Uniform Buffer 和描述符

目标：

- 创建 Uniform Buffer。
- 创建 Descriptor Set Layout、Descriptor Pool、Descriptor Set。
- 每帧更新 MVP 矩阵。

存档目录：

```text
archives/12-uniform-buffer-descriptors/
```

完成标准：

- 渲染对象可以随时间旋转或变换。
- 中文注释说明 Descriptor Set 如何把 CPU 侧资源绑定到 shader。

### 13. 纹理

目标：

- 加载图片。
- 创建 Vulkan Image。
- 创建 Sampler。
- 在 shader 中采样纹理。

存档目录：

```text
archives/13-texture/
```

完成标准：

- 三角形或矩形可以显示纹理。
- 中文注释说明 Image Layout 转换和 Sampler 的作用。

### 14. 深度缓冲

目标：

- 创建 Depth Image。
- 配置深度测试。
- 更新 Render Pass 和 Framebuffer。

存档目录：

```text
archives/14-depth-buffer/
```

完成标准：

- 3D 物体遮挡关系正确。
- 中文注释说明深度格式选择和深度附件生命周期。

### 15. 模型加载

目标：

- 加载简单 `.obj` 模型。
- 上传模型顶点和索引。
- 渲染模型。

存档目录：

```text
archives/15-model-loading/
```

完成标准：

- 能显示一个外部模型。
- README 记录模型来源或自制说明。
- 中文注释说明模型数据如何转换为 Vulkan Buffer。

### 16. 项目整理和复盘

目标：

- 清理重复代码。
- 抽取 Vulkan 对象生命周期管理。
- 整理学习笔记。

存档目录：

```text
archives/16-review-refactor/
```

完成标准：

- 主线代码结构更清晰。
- 每个阶段存档完整。
- README 能指引新读者从零开始运行项目。

## 每阶段 README 模板

每个存档目录中的 `README.md` 使用以下结构：

```markdown
# 阶段编号：阶段名称

## 学习目标

说明这一阶段要理解的 Vulkan 概念。

## 关键概念

列出本阶段涉及的 Vulkan 对象和它们之间的关系。

## 构建方式

写明 CMake 配置和构建命令。

## 运行方式

写明可执行文件位置和运行方式。

## 代码说明

说明关键代码路径，并解释中文注释覆盖的重点。

## 常见问题

记录本阶段遇到的错误、验证层提示和解决方法。

## 下一步

说明下一阶段准备学习什么。
```
