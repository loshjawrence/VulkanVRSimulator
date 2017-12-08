#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D ColorSampler;
layout(binding = 2) uniform sampler2D DepthSampler;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view[2];
    mat4 proj;
    float time;
} ubo;

layout (push_constant) uniform PerDrawCallInfo {
    mat4 timeWarpInvVP;
    int toggleFlags;
    int renderTargetWidth;
    int renderTargetHeight;
} PushConstant;
const int camBit = 1;
const int vrBit = 0;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNor;
layout(location = 4) in vec3 inTan;
layout(location = 5) in vec3 inBiTan;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNor;
layout(location = 3) out vec3 fragTan;
layout(location = 4) out vec3 fragBiTan;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    const int vrMode = (PushConstant.toggleFlags >> vrBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    //if vrMode(this shader only enabled in vrMode anyway), shrink UV.x by half and shift to sample one eye
    vec2 uv = inTexCoord;
    uv.x = (uv.x * (1.f - 0.5*vrMode)) + 0.5f*camIndex;
    fragTexCoord    = uv;

    //convert inPosition to ndc with depth
    const mat4 vp = ubo.proj * ubo.view[camIndex];
    const vec3 ndc = vec3(inPosition.x, inPosition.y, texture(DepthSampler, uv).x);
    const vec4 worldPos = PushConstant.timeWarpInvVP * vec4(ndc,1.f);
    const vec4 updatedCamPos = vp * worldPos;
    gl_Position     = updatedCamPos;
//    fragColor = vec3(texture(ColorSampler, uv));
//    gl_Position     = vec4(inPosition, 1.0);
//    fragColor = vec3(depth.x, depth.x, depth.x);
}
