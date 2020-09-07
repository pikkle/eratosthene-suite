#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform MatrixBuffer {
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    // @TODO: convert from WGS (lon, lat, alt) in DOUBLE PREC. to cartesian (x, y, z) in SINGLE PREC.
    // @TODO: then apply uniform matrices on (x, y, z) (model, view and projection).
    // model matrix moves/rotates/scales the object model
    // view matrix moves/rotates/scales the world around the camera
    // projection matrix adds a perspective on the world and gives a sense of depth on a 2d plane

    gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPosition, 1.0);
    gl_PointSize = 5.0;

    fragColor = inColor;
}