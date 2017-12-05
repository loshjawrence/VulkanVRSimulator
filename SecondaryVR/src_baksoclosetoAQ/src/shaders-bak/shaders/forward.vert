#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 0) uniform UniformBufferObject {
    mat4 view[2];
    mat4 proj;
    mat4 viewProj[2];
    vec3 viewPos;
    vec3 lightPos;
    float time;
} ubo;

layout (push_constant) uniform PerDrawCallInfo {
    mat4 model;
    int toggleFlags;
} PushConstant;
const int camBit = 1;
const int dynamicBit = 0;

/////////////
//// IN  ////
/////////////
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNor;
layout(location = 4) in vec3 inTan;
layout(location = 5) in vec3 inBiTan;

/////////////
//// OUT ////
/////////////
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 tanFragPos;
layout(location = 3) out vec3 tanViewPos;
layout(location = 4) out vec3 tanLightPos;
layout(location = 5) out vec3 tanNor;

///////////////////
///// BUILT-IN ////
///////////////////
out gl_PerVertex {
    vec4 gl_Position;
};

//HELPER
mat4 rotationMatrix(vec3 axis, const float angle);
mat4 rotationMatrixBasic(const float angle);

void main() {
    //use flags to determine certain states
    const int isDynamic = (PushConstant.toggleFlags >> dynamicBit) & 1;
    const int camIndex = (PushConstant.toggleFlags >> camBit) & 1;

    //get persp-proj pos
    const mat4 updatedModelMatrix = PushConstant.model * rotationMatrix(vec3(0.f, 1.f, 0.f), 1.f * ubo.time * 3.1415f/4.f);
    const vec3 worldPos = vec3(updatedModelMatrix * vec4(inPos, 1.0));   
    gl_Position = ubo.viewProj[camIndex] * vec4(worldPos, 1.f);


    //pass along the color and uv
    fragColor       = inColor;
    fragTexCoord    = inTexCoord;




    mat3 normalMatrix = transpose(inverse(mat3(updatedModelMatrix)));
    vec3 T = normalize(normalMatrix * inTan);
    vec3 N = normalize(normalMatrix * inNor);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));    
    tanLightPos = TBN * ubo.lightPos;
    tanViewPos  = TBN * ubo.viewPos;
    tanFragPos  = TBN * worldPos;
    tanNor      = inNor;
        

//    //Find worldToTan transform
//    mat3 toWorldNormalMatrix = transpose(inverse(mat3(updatedModelMatrix)));
//    vec3 worldN     = normalize(toWorldNormalMatrix * inNor);
//    vec3 worldT     = normalize(toWorldNormalMatrix * inTan);
//    worldT          = normalize(worldT - dot(worldT, worldN) * worldN);//ensure ortho
//    vec3 worldB     = cross(worldN, worldT);
//    mat3 worldToTan = transpose(mat3(worldT, worldB, worldN));    
//
//    //transform to tangent space to remove transformations in frag shader if it has normal map
//    tanFragPos  = worldToTan * worldPos;
//    tanViewPos  = worldToTan * ubo.viewPos;
//    tanLightPos = worldToTan * ubo.lightPos;
//    tanNor = inNor;
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
