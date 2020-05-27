#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 texCoord;
layout(set = 0, binding = 1) uniform sampler texSampler;
layout(set = 1, binding = 1) uniform texture2D baseTex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(baseTex, texSampler), texCoord);
}