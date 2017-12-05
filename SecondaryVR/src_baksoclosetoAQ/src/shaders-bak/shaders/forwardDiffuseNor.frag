#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(early_fragment_tests) in;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D norSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 tanFragPos;
layout(location = 3) in vec3 tanViewPos;
layout(location = 4) in vec3 tanLightPos;
layout(location = 5) in vec3 tanNor;

layout(location = 0) out vec4 outColor;

void main() {
     // obtain normal from normal map in range [0,1]
    vec3 normal = texture(norSampler, fragTexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);  // transform normal vector to range [-1,1]// this normal is in tangent space

    // get diffuse color
    vec3 color = texture(texSampler, fragTexCoord).rgb;

    // ambient
    vec3 ambient = 0.3 * color;

    // diffuse
    vec3 lightDir = normalize(tanLightPos - tanFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    // specular
    vec3 viewDir = normalize(tanViewPos - tanFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.2) * spec;

    //final color
    outColor = vec4(ambient + diffuse + specular, 1.0);
}


//layout(location = 2) in vec3 worldNor;
//layout(location = 3) in vec3 worldTan;
//layout(location = 4) in vec3 worldBiTan;
//
//layout(location = 0) out vec4 outColor;
//
//void main() {
//	const vec3 lightdir = normalize(vec3(0.f, 1.f, 0.f));
//	const vec3 color = texture(texSampler, fragTexCoord).xyz;
//	const float ambient = 0.5f;
//	const vec3 lambert = color * clamp(dot(lightdir, worldNor), ambient, 1.f);
//	outColor = vec4(lambert.xyz, 1.f);
//
////    outColor = texture(texSampler, fragTexCoord);
//}
