#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(early_fragment_tests) in;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 worldNor;
layout(location = 3) in vec3 worldTan;
layout(location = 4) in vec3 worldBiTan;

layout(location = 0) out vec4 outColor;

void main() {
	const vec3 lightdir = normalize(vec3(0.f, 1.f, 0.f));
	const vec3 color = fragColor.xyz;
	const float ambient = 0.5f;
	const vec3 lambert = color * clamp(dot(lightdir, worldNor), ambient, 1.f);
	outColor = vec4(lambert.xyz, 1.f);
//    outColor = vec4(color.xyz,1.f);
}

