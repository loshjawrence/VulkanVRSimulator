#pragma once
#include "Mesh.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

#include "VulkanBuffer.h"


Mesh::Mesh() {
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, 
	const std::vector<Texture>& textures, const VulkanContextInfo& contextInfo)
	: mVertices(vertices), mIndices(indices), mTextures(textures)
{
	setupVulkanBuffers(contextInfo);
}

Mesh::~Mesh() {
}

void Mesh::createDescriptor(const VulkanContextInfo& contextInfo, const VkBuffer& ubo, const uint32_t sizeofUBOstruct) {
	std::vector<int> diffuseindices;
	std::vector<int> specindices;
	std::vector<int> norindices;
	std::vector<int> heightindices;
	//bind textures for this mesh
	if (mTextures.size() > 4) {
		std::cout << "\nMore than 4 textures in mesh" << std::endl;
	}
	for (unsigned int i = 0; i < mTextures.size(); ++i) {
		std::string name = mTextures[i].type;
		if (name == "texture_diffuse") {
			diffuseindices.push_back(i);
		} else if (name == "texture_specular") {
			specindices.push_back(i);
		} else if (name == "texture_normal") {
			norindices.push_back(i);
		} else if (name == "texture_height") {
			heightindices.push_back(i);
		} else {
			std::cout << "Not sure which texture type this is: " << name << std::endl;
		}
	}

	//TODO:select the example descrptor on which we will base the member descriptor
	//if there's no supporting example descriptor(and therefore no supporting pipeline)
	//create a new one just for this mesh?
	descriptor.determineDescriptorType(diffuseindices.size(), specindices.size(), norindices.size(), heightindices.size());


	//VulkanDescriptor has static members for the differnt layouts which get created at init
	//the set is created based on the type

	descriptor.createDescriptorSetLayout(contextInfo);
	descriptor.createDescriptorPool(contextInfo);//each should have their own pool
	descriptor.createDescriptorSet(contextInfo,ubo,sizeofUBOstruct,mTextures[diffuseindices[0]].vulkanImage);

}


void Mesh::setupVulkanBuffers(const VulkanContextInfo& contextInfo) {
	//setup vulkan buffers for the geometry
	VulkanBuffer::createVertexBuffer(contextInfo, mVertices, vertexBuffer, vertexBufferMemory);
	VulkanBuffer::createIndexBuffer(contextInfo, mIndices, indexBuffer, indexBufferMemory);
}
