#version 450 core
in vec3 worldnor;
out vec4 FragColor;

void main() {
	const vec3 lightdir = vec3(0.f, 1.f, 0.f);
	const vec3 color = vec3(1.f, 1.f, 1.f);
	const float ambient = 0.1f;
	const vec3 lambert = clamp(color * dot(lightdir, worldnor), ambient, 1.f);
	FragColor = vec4(lambert.xyz, 1.f);
}
