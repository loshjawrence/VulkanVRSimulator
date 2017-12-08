#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 1) uniform sampler2D ColorSampler;
layout(binding = 2) uniform sampler2D DepthSampler;

layout (push_constant) uniform PerDrawCallInfo {
    mat4 timeWarpInvVP;
    int toggleFlags;
    int width;
    int height;
} PushConstant;
const int camBit = 1;
const int vrBit = 0;


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragNor;
layout(location = 3) in vec3 fragTan;
layout(location = 4) in vec3 fragBiTan;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(ColorSampler, fragUV);
}

