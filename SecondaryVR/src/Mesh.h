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
	NDCTRIANGLE = 0, NDCBARRELMESH, NDCBARRELMESH_PRECALC
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


	//for barrel mesh
	uint32_t quadsPerDim = 20;

public: 
	Mesh(const VulkanContextInfo& contextInfo, const MESHTYPE, uint32_t camIndex = 0);
	Mesh();
	~Mesh();
	Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
		const std::vector<Texture>& textures, const VulkanContextInfo& contextInfo);
	void createDescriptor(const VulkanContextInfo& contextInfo, const VkBuffer& ubo, const uint32_t sizeofUBOstruct);
	void createNDCTriangle(const VulkanContextInfo& contextInfo);
	void createNDCBarrelMesh(const VulkanContextInfo& contextInfo, const uint32_t camIndex);
	void createNDCBarrelMeshPreCalc(const VulkanContextInfo& contextInfo, const uint32_t camIndex);

private:
	void genGridMesh(const VulkanContextInfo& contextInfo, const uint32_t camIndex, const uint32_t shift);
	void setupVulkanBuffers(const VulkanContextInfo& contextInfo);
	//void Mesh::getSourceUV(const uint32_t camIndex, const glm::vec2& oTexCoord,
	//	glm::vec2& out_tcRed, glm::vec2& out_tcGreen, glm::vec2& out_tcBlue);
	//glm::vec2 convertRadToUV(const glm::vec2 normalized_ndc, const float radius, const uint32_t camIndex);
	//float convertUVToRad(const glm::vec2 uv, const uint32_t camIndex);
	//glm::vec3 distortInverse(glm::vec3 ndc, const uint32_t camIndex);
private:
};
