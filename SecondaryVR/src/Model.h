#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "stb_image.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include "Mesh.h"

class Model {
public:
	std::vector<Texture> mTexturesLoaded;//models will likely use textures more that once, only load it once
	std::vector<Mesh> mMeshes;
	std::string mDirectory;
	bool culled = false;
	glm::mat4 modelMatrix;
	uint32_t isDynamic = 0;
	std::string path;

	//ubo
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

public:
	Model();
	~Model();
	Model(const std::string& path, uint32_t isDynamic, const glm::mat4& model, VulkanContextInfo& contextInfo, 
		const VkBuffer& ubo, const VkDeviceMemory& uboMemory, const uint32_t sizeofUBOstruct);

	void loadModel(const std::string& path, const VulkanContextInfo& contextInfo);
	void createDescriptorsForMeshes(const VulkanContextInfo& contextInfo, 
		const VkBuffer& ubo, const uint32_t sizeofUBOstruct);

	void destroyVulkanHandles(const VulkanContextInfo& contextInfo);

private:
	void processNode(aiNode* node, const aiScene const* scene, const VulkanContextInfo& contextInfo);
	Mesh processMesh(const aiMesh* mesh, const aiScene* scene, const VulkanContextInfo& contextInfo);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const VulkanContextInfo& contextInfo);

	//unsigned int TextureFromFile(const char* path, const std::string& dir);
};
