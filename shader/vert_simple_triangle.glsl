#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBuffer {
    mat4 model;
    mat4 view;
    mat4 proj;
} uniform;

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = uniform.model * uniform.view * uniform.proj * vec4(position, 0.0, 1.0);
    fragColor = color;
}