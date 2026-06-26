# Vulkan 学习阶段复盘

## 阶段链路

01 到 05 阶段建立窗口显示基础：Instance、验证层、Surface、Device、Queue、Swapchain。

06 到 09 阶段建立最小渲染闭环：Image View、Render Pass、Pipeline、Framebuffer、Command Buffer、Semaphore、Fence。

10 到 13 阶段建立资源输入：Vertex Buffer、Index Buffer、Uniform Buffer、Descriptor Set、Texture Image、Sampler。

14 到 15 阶段接近常见 3D 渲染路径：Depth Buffer 和 OBJ 模型加载。

## 生命周期顺序

创建大致顺序：

```text
Window
Instance
Debug Messenger
Surface
Physical Device
Logical Device / Queues
Swapchain
Image Views
Render Pass
Descriptor Set Layout
Pipeline
Command Pool
Buffers / Images / Descriptors
Framebuffers
Command Buffers
Sync Objects
```

销毁时按反向依赖释放。特别要注意：

- Framebuffer 依赖 Image View、Depth Image View 和 Render Pass。
- Pipeline 依赖 Render Pass 和 Pipeline Layout。
- Descriptor Set 依赖 Descriptor Pool，Descriptor Set 使用的 Buffer/Image/Sampler 要在 GPU 空闲后释放。
- Command Buffer 可能引用 Buffer、Framebuffer、Pipeline、Descriptor Set，因此清理前要等待设备空闲。

## 后续重构建议

优先重构资源生命周期，而不是先追求复杂功能：

- 用 RAII 包装 `VkBuffer + VkDeviceMemory`。
- 用 RAII 包装 `VkImage + VkDeviceMemory + VkImageView`。
- 把一次性命令缓冲封装为上传工具。
- 把 Descriptor Set Layout、Pool、Set 的创建集中管理。
- 把 Swapchain 相关对象放到可重建结构中，为窗口 resize 做准备。
