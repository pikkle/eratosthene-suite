#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) buffer UniformBufferObject {
    mat4 view;
    mat4 proj;
//    uint modelCount;
//    mat4 models[]; // array of all cells transformation matrices
    mat4 models; // array of all cells transformation matrices
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in uint inCell;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.models * vec4(inPosition, 1.0);
    gl_Position = vec4((gl_Position.xyz / gl_Position.w) / 4.0, 1.0);
    gl_PointSize = 1.0;

    fragColor = inColor;
}