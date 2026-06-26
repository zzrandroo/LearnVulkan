#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // 固定输出一组容易辨认的颜色，先验证片元阶段和颜色附件配置能连起来。
    outColor = vec4(0.95, 0.28, 0.18, 1.0);
}
