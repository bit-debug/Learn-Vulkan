#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 inVertexColor;
layout(location = 1) in vec2 inTexturePosition;
layout(location = 0) out vec4 outColor;

void main() {
    // outColor = vec4(inTexturePosition, 0.0, 1.0);
    outColor = texture(textureSampler, inTexturePosition);
}