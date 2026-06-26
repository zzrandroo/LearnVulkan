#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    // 使用顶点颜色插值后的结果，方便确认 Vertex Buffer 中的位置和颜色都正确进入了管线。
    outColor = vec4(fragColor, 1.0);
}
