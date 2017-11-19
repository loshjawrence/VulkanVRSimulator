#version 450 core

in vec2 TexCoords;
in vec3 worldnor;

out vec4 FragColor;

// texture samplers
uniform sampler2D texture_diffuse1;

void main() {
	// linearly interpolate between both textures (80% container, 20% awesomeface)
	//FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
	const vec3 lightdir = normalize(vec3(0.f, 1.f, 0.f));
	const vec3 color = texture(texture_diffuse1, TexCoords).xyz;
	const float ambient = 0.5f;
	const vec3 lambert = color * clamp(dot(lightdir, worldnor), ambient, 1.f);
	FragColor = vec4(lambert.xyz, 1.f);
	//FragColor = texture(texture_diffuse1, TexCoords);
	//FragColor = vec4(1.f, 0.f, 0.f, 1.f);
}