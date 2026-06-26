#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    // 第十阶段开始从 Vertex Buffer 读取顶点属性。
    // position 决定三角形顶点位置，color 会被光栅化阶段插值后传给片元着色器。
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
