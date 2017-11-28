#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 0) uniform UniformBufferObject {
//    mat4 view[2];
//    mat4 proj;
//    float time;
//} ubo;
//
//layout (push_constant) uniform PerDrawCallInfo {
//    mat4 model;
//    int toggleFlags;
//} PushConstant;

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
    gl_Position     = vec4(inPosition, 1.0);
    fragTexCoord    = inTexCoord;
}
