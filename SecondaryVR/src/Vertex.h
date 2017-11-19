#pragma once

#include "VulkanDevices.h"
#include "VulkanSwapChain.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanRenderPass.h"

#include <vector>
#include <array>
#include <string>
#include <glm/glm.hpp>

class Vertex {
public:
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
public:
	Vertex();
	Vertex(glm::vec3& pos, glm::vec3& color, glm::vec2& texCoord);
	~Vertex();
	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
	bool operator==(const Vertex& other) const;
};
//

