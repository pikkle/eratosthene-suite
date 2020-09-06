#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform ViewProjBuffer {
    mat4 view;
    mat4 proj;
} vpBuffer;

layout(binding = 1) buffer ModelsBuffer {
    uint modelCount;
    mat4 models[32767]; // array of all cells transformation matrices
}  mBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in uint inCell;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vpBuffer.proj * vpBuffer.view * mBuffer.models[inCell] * vec4(inPosition, 1.0);
    gl_PointSize = 1.0;

    fragColor = inColor;
}