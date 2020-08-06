#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) buffer UniformBufferObject {
    mat4 view;
    mat4 proj;
    uint modelCount;
    mat4 models[]; // array of all cells transformation matrices
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in uint inCell;

layout(location = 0) out vec3 fragColor;

void main() {
    if (inCell < ubo.modelCount) {
        gl_Position = ubo.proj * ubo.view * ubo.models[inCell] * vec4(inPosition, 1.0);
    } else {
        gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    }
    gl_PointSize = 2.0;
    fragColor = inColor;
}