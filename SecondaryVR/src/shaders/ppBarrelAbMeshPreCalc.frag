#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler;

layout (push_constant) uniform PerDrawCallInfo {
    int toggleFlags;
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
    const int vrMode = (PushConstant.toggleFlags >> vrBit) & 1;
    //const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    const vec2 tcRed    = fragColor.xy;
    const vec2 tcGreen  = fragUV;
    const vec2 tcBlue   = fragTan.xy;

    if(1 == vrMode) {
        outColor = vec4(texture(texSampler, tcRed).r,
                        texture(texSampler, tcGreen).g,
                        texture(texSampler, tcBlue).b, 1.f);
    } else {
        outColor = texture(texSampler, fragUV);
    }
}




