#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec2 inVertexPosition;
layout(location = 1) in vec3 inVertexColor;
layout(location = 2) in vec2 inTexturePosition;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexturePosition;

void main() {
    // gl_Position = vec4(inVertexPosition, 0.0, 1.0);
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inVertexPosition, 0.0, 1.0);
    outColor = inVertexColor;
    outTexturePosition = inTexturePosition;
}