#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
} ubo;

void main() {
    // 第十二阶段通过 Descriptor Set 读取 Uniform Buffer。
    // mvp 矩阵每帧由 CPU 更新，让顶点位置随时间旋转。
    gl_Position = ubo.mvp * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
