#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (push_constant) uniform ModelInfo {
    mat4 model;
    float time;
    float dynamic;
} PushConstant;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

mat4 rotationMatrix(vec3 axis, const float angle);
mat4 rotationMatrixBasic(const float angle);

void main() {
//    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * PushConstant.model *
        rotationMatrixBasic(PushConstant.time * 3.1415f/4.f) * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

mat4 rotationMatrixBasic(const float angle) {
    float s = sin(angle);
    float c = cos(angle);
    
    //column major?
    return mat4( 1.f, 0.f, 0.f,  0.f,
                 0.f,   c,   s,  0.f,
                 0.f,  -s,   c,  0.f,
                 0.f,  0.f, 0.f, 1.f);
}

mat4 rotationMatrix(vec3 axis, const float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}
