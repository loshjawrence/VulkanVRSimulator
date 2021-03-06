#pragma once

#include "VulkanContextInfo.h"

#include <array>
#include <glm/glm.hpp>

class Vertex {
public:
    glm::vec3 pos;
	glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
	glm::vec2 uv;
	glm::vec3 nor;
	glm::vec3 tan;
	glm::vec3 bitan;

public:
	Vertex();
	Vertex(glm::vec3& pos, glm::vec3& color, glm::vec2& uv, 
		glm::vec3& nor, glm::vec3& tan, glm::vec3 bitan);
	~Vertex();
	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions();
	bool operator==(const Vertex& other) const;
};
//

