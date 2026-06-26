#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D textureSampler;

void main() {
    // 纹理颜色来自 sampler，顶点颜色作为轻微调制，方便同时确认 UV 和顶点属性都进入了 shader。
    outColor = texture(textureSampler, fragTexCoord) * vec4(fragColor, 1.0);
}
