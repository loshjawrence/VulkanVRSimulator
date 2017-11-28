#pragma once
#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Vertex.h"
#include "VulkanImage.h"
#include "VulkanDescriptor.h"


struct Texture {
	//uint32_t id;//gpu bound opaque ref
	VulkanImage vulkanImage;
	std::string type;//diffuse, specular, normal, height, ..
	aiString path;//file path
	bool duplicate = false;
};

enum class MESHTYPE {
	NDCTRIANGLE = 0, NDCBARRELMESH,
};

class Mesh {
public: 
	std::vector<Vertex> mVertices;
	std::vector<unsigned int> mIndices;
	std::vector<Texture> mTextures;

	//vulkan vertex
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	//vulkan index
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VulkanDescriptor descriptor;

	std::vector<int> diffuseindices;
	std::vector<int> specindices;
	std::vector<int> norindices;
	std::vector<int> heightindices;

public: 
	Mesh(const VulkanContextInfo& contextInfo, const MESHTYPE);
	Mesh();
	~Mesh();
	Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
		const std::vector<Texture>& textures, const VulkanContextInfo& contextInfo);
	void createDescriptor(const VulkanContextInfo& contextInfo, const VkBuffer& ubo, const uint32_t sizeofUBOstruct);
	void createNDCTriangle(const VulkanContextInfo& contextInfo);
	void createNDCBarrelMesh(const VulkanContextInfo& contextInfo);

private:
	void setupVulkanBuffers(const VulkanContextInfo& contextInfo);
};
