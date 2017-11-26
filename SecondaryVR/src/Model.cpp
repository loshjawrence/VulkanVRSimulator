#pragma once
#include "Model.h"

#include "VulkanContextInfo.h"
#include "VulkanBuffer.h"


Model::Model() { }
Model::~Model() { }

Model::Model(const std::string& path, const uint32_t isDynamic, const glm::mat4& modelMatrix, VulkanContextInfo& contextInfo, 
	const VkBuffer& ubo, const VkDeviceMemory& uboMemory, const uint32_t sizeofUBOstruct)
	:path(path), isDynamic(isDynamic), modelMatrix(modelMatrix)
{
	loadModel(path, contextInfo);
	createDescriptorsForMeshes(contextInfo, ubo, sizeofUBOstruct);
}

Model::Model(const std::string& path, const bool isDyamic, const glm::mat4& modelMatrix, VulkanContextInfo& contextInfo)
	:path(path), isDynamic(isDynamic), modelMatrix(modelMatrix)
{
	VulkanBuffer::createUniformBuffer(contextInfo, sizeof(UniformBufferObject2), uniformBuffer, uniformBufferMemory);
	loadModel(path, contextInfo);
	createDescriptorsForMeshes(contextInfo);
}

void Model::loadModel(const std::string& path, const VulkanContextInfo& contextInfo) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ValidateDataStructure);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
		return;
	}

	mDirectory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene, contextInfo);
}

void Model::processNode(aiNode* node, const aiScene const* scene, const VulkanContextInfo& contextInfo) {
	//process each mesh located at current node
	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
		//the node object only contains indices to retrieve te mesh out of the main mMeshes array in scene
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		mMeshes.push_back(processMesh(mesh, scene, contextInfo));
	}

	//process the children of this node
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		processNode(node->mChildren[i], scene, contextInfo);
	}
}

Mesh Model::processMesh(const aiMesh* mesh, const aiScene* scene, const VulkanContextInfo& contextInfo) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	//vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vertex;
		vertex.pos = glm::vec3(mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z);
		vertex.nor = glm::vec3(mesh->mNormals[i].x,
			mesh->mNormals[i].y,
			mesh->mNormals[i].z);
		if (mesh->mTextureCoords[0]) {//any texture coords?
			//a vertex can contain up to 8 different texture coords. we'll only use the first set (0)
			vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y);
		} else {
			vertex.uv = glm::vec2(0.f, 0.f);
		}

		//we told it to generate tangent space so lets grab them
		if (mesh->mTangents != nullptr && mesh->mBitangents != nullptr) {
			vertex.tan = glm::vec3(mesh->mTangents[i].x,
				mesh->mTangents[i].y,
				mesh->mTangents[i].z);
			vertex.bitan = glm::vec3(mesh->mBitangents[i].x,
				mesh->mBitangents[i].y,
				mesh->mBitangents[i].z);
		} else {
			vertex.tan = glm::vec3(0.f, 0.f, 0.f);
			vertex.bitan = glm::vec3(0.f, 0.f, 0.f);
		}
		vertices.push_back(vertex);
	}

	//indices
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	/* Materials: we assume a convention for sampler names in the shaders.
	each diffuse texture should be named as "texture_diffuseN" where
	N is a number from 1 to MAX_SAMPLER_NUMBER
	diffuse: texture_diffuseN
	specular: texture_specularN
	normal: texture_normalN
	*/
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	//diffuse maps
	std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, contextInfo);
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

	//spec maps
	std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, contextInfo);
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

	//normal maps
	std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, contextInfo);
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

	//height maps
	std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, contextInfo);
	textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

	//std::move this, std::move the args in the constructor
	if (textures.size() == 0) {
		int aieinfo = 0;
	}
	return Mesh(vertices, indices, textures, contextInfo);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, 
	aiTextureType type, const VulkanContextInfo& contextInfo) {
	std::string typeName;
	if		(type == aiTextureType_DIFFUSE)		typeName = "texture_diffuse";
	else if (type == aiTextureType_SPECULAR)	typeName = "texture_specular";
	else if (type == aiTextureType_NORMALS)		typeName = "texture_normal";
	else if (type == aiTextureType_HEIGHT)		typeName = "texture_height";
	std::vector<Texture> textures;
	const unsigned int numtextures = mat->GetTextureCount(type);
	for (unsigned int i = 0; i < numtextures; ++i) {
		aiString str;
		mat->GetTexture(type, i, &str);
		//check if texture was loaded before, if so continue
		bool skip = false;
		for (unsigned int j = 0; j < mTexturesLoaded.size(); ++j) {
			if (std::strcmp(mTexturesLoaded[j].path.C_Str(), str.C_Str()) == 0) {
				textures.push_back(mTexturesLoaded[j]);
				textures.back().duplicate = true;
				skip = true;
				break;
			}
		}

		if (skip) { continue; }

		Texture texture; const VkExtent2D defaultextent = { 0, 0 };
		std::string texturepath(mDirectory + '/' + std::string(str.C_Str()));
		texture.vulkanImage = VulkanImage(IMAGETYPE::TEXTURE, defaultextent, VK_FORMAT_R8G8B8A8_UNORM, contextInfo, texturepath);
		texture.type = typeName;
		texture.path = str;
		textures.push_back(texture);
		mTexturesLoaded.push_back(texture);
	}
	return textures;
}

void Model::createDescriptorsForMeshes(const VulkanContextInfo& contextInfo,
	 const VkBuffer& ubo, const uint32_t sizeofUBOstruct) {

	for (auto& mesh : mMeshes) {
		mesh.createDescriptor(contextInfo, ubo, sizeofUBOstruct);
	}

}
void Model::createDescriptorsForMeshes(const VulkanContextInfo& contextInfo) {
	for (auto& mesh : mMeshes) {
		mesh.createDescriptor(contextInfo, uniformBuffer, sizeof(UniformBufferObject2));
	}
}
void Model::destroyVulkanHandles(const VulkanContextInfo& contextInfo) {
	for (auto& mesh : mMeshes) {
		for (auto& texture : mesh.mTextures) {
			if (texture.duplicate)continue;
			texture.vulkanImage.destroyImage(contextInfo);
		}
		mesh.descriptor.destroyVulkanDescriptor(contextInfo);
	}
	vkDestroyBuffer(contextInfo.device, uniformBuffer, nullptr);
	vkFreeMemory(contextInfo.device, uniformBufferMemory, nullptr);

	for (auto& mesh : mMeshes) {
		vkDestroyBuffer(contextInfo.device, mesh.indexBuffer, nullptr);
		vkFreeMemory(contextInfo.device, mesh.indexBufferMemory, nullptr);
		vkDestroyBuffer(contextInfo.device, mesh.vertexBuffer, nullptr);
		vkFreeMemory(contextInfo.device, mesh.vertexBufferMemory, nullptr);
	}
}
