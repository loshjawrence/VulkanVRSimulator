#include "Vertex.h"

Vertex::Vertex()
	//: pos(glm::vec3(0.f)), color(glm::vec3(0.f)), texCoord(glm::vec2(0.f))
	//NEW
	: pos(glm::vec3(0.f)), color(glm::vec3(0.f)), uv(glm::vec2(0.f)),
	nor(glm::vec3(0.f)), tan(glm::vec3(0.f)), bitan(glm::vec3(0.f))
{
}


//Vertex::Vertex(glm::vec3& pos, glm::vec3& color, glm::vec2& texCoord)
//	: pos(pos), color(color), texCoord(texCoord)
//{
//}
//NEW
Vertex::Vertex(glm::vec3& pos, glm::vec3& color, glm::vec2& uv,
	glm::vec3& nor, glm::vec3& tan, glm::vec3 bitan) 
	: pos(pos), color(color), uv(uv), nor(nor), tan(tan), bitan(bitan)
{
}

Vertex::~Vertex() {
}

VkVertexInputBindingDescription Vertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

//std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
//	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
//
//	attributeDescriptions[0].binding = 0;
//	attributeDescriptions[0].location = 0;
//	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
//	attributeDescriptions[0].offset = offsetof(Vertex, pos);
//
//	attributeDescriptions[1].binding = 0;
//	attributeDescriptions[1].location = 1;
//	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
//	attributeDescriptions[1].offset = offsetof(Vertex, color);
//
//	attributeDescriptions[2].binding = 0;
//	attributeDescriptions[2].location = 2;
//	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
//	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
//
//	return attributeDescriptions;
//}
//NEW
std::array<VkVertexInputAttributeDescription, 6> Vertex::getAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions = {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, uv);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, nor);

	attributeDescriptions[4].binding = 0;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(Vertex, tan);

	attributeDescriptions[5].binding = 0;
	attributeDescriptions[5].location = 5;
	attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[5].offset = offsetof(Vertex, bitan);

	return attributeDescriptions;
}

bool Vertex::operator==(const Vertex& other) const {
	//return pos == other.pos && color == other.color && texCoord == other.texCoord;
	//NEW
	return pos == other.pos && color == other.color && uv == other.uv &&
		   nor == other.nor && tan == other.tan && bitan == other.bitan;
}
