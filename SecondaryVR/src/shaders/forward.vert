#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 0) uniform UniformBufferObject {
    mat4 view[2];
    mat4 proj;
    float time;
} ubo;

layout (push_constant) uniform PerDrawCallInfo {
    mat4 model;
    int toggleFlags;
} PushConstant;
const int camBit = 1;
const int dynamicBit = 0;

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

mat4 rotationMatrix(vec3 axis, const float angle);
mat4 rotationMatrixBasic(const float angle);

void main() {
    const int isDynamic = (PushConstant.toggleFlags >> dynamicBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    mat4 updatedModelMatrix = PushConstant.model * rotationMatrix(vec3(0.f, 1.f, 0.f), isDynamic * ubo.time * 3.1415f/4.f);
    gl_Position = ubo.proj * ubo.view[camIndex] * updatedModelMatrix * vec4(inPosition, 1.0);

    fragColor       = inColor;
    fragTexCoord    = inTexCoord;
	fragNor         = normalize(transpose(inverse(mat3(updatedModelMatrix))) * inNor);
    fragTan         = normalize(mat3(updatedModelMatrix) * inTan);
    fragBiTan       = normalize(mat3(updatedModelMatrix) * inBiTan);

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
