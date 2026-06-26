#version 450

// 第八阶段仍然不引入 Vertex Buffer，所以直接用 gl_VertexIndex 生成三角形顶点。
// 这样可以先专注理解命令录制流程，顶点数据上传会留到后续阶段学习。
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
