#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNor;
layout(location = 3) in vec3 fragTan;
layout(location = 4) in vec3 fragBiTan;

layout(location = 0) out vec4 outColor;

void main() {
    const vec3 color = vec3(1.f, 1.f, 1.f);
	const vec3 lightdir = vec3(1.f, 1.f, 1.f);
	const float ambient = 0.1f;
	const vec3 lambert = clamp(color * dot(normalize(lightdir), fragNor), ambient, 1.f);

//    outColor = vec4(lambert.xyz,1.f);
    outColor = vec4(color.xyz,1.f);
//    outColor = vec4(fragNor.xyz,1.f);
}

