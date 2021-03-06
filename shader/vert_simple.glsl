#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformMatrix {
    mat4 view;
    mat4 proj;
} matrix;

layout(push_constant) uniform MeshConstant {
    mat4 model;
} pc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = matrix.proj * matrix.view * pc.model * vec4(position, 1.0);
    fragColor = color;
    fragTexCoord = texCoord;
}