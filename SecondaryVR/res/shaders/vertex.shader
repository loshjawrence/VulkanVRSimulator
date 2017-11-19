#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 worldnor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	TexCoords = aTexCoords;
	worldnor = normalize(transpose(inverse(mat3(model))) * aNormal);
	gl_Position = projection * view * model * vec4(aPos, 1.f);
}



//#shader vertex
//#version 330 core
//
//layout(location = 0) in vec4 position;
//
//void main() {
//	gl_Position = position;
//};
//
//#shader fragment
//#version 330 core
//
//layout(location = 0) out vec4 color;
//
//void main() {
//	color = vec4(1.0, 0.0, 0.0, 1.0);
//};
