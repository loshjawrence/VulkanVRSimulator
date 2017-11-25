#pragma once
#include "VulkanDescriptor.h"
#include "Mesh.h"
#include <array>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

std::vector<VkDescriptorSetLayout> VulkanDescriptor::layoutTypes = 
std::vector<VkDescriptorSetLayout>(VulkanDescriptor::MAX_IMAGESAMPLERS + 1);

bool VulkanDescriptor::layoutsInitialized = false;

void initDescriptorSetLayoutTypes(const VulkanContextInfo& contextInfo) {
	VulkanDescriptor::layoutTypes.reserve(VulkanDescriptor::MAX_IMAGESAMPLERS + 1);
	std::vector<VkDescriptorSetLayoutBinding> bindings = {}; 

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


	bindings.push_back(uboLayoutBinding);

	//Make a no-texture descriptor layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(contextInfo.device, &layoutInfo, nullptr, &VulkanDescriptor::layoutTypes[0]) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create descriptor set layout!";
		throw std::runtime_error(ss.str());
	}

	//make various layouts using 1 to MAX_IMAGESAMPLERS number of image samplers
	//store them in the static vector layoutTypes
	for (uint32_t i = 1; i < VulkanDescriptor::MAX_IMAGESAMPLERS + 1; ++i) {
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = i;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(samplerLayoutBinding);

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(contextInfo.device, &layoutInfo, nullptr, &VulkanDescriptor::layoutTypes[i]) != VK_SUCCESS) {
			std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create descriptor set layout!";
			throw std::runtime_error(ss.str());
		}
	}

	VulkanDescriptor::layoutsInitialized = true;
}

VulkanDescriptor::VulkanDescriptor() {
}

VulkanDescriptor::VulkanDescriptor(const VulkanContextInfo& contextInfo)
{
	createDescriptorSetLayout(contextInfo);
	createDescriptorPool(contextInfo);
}

VulkanDescriptor::~VulkanDescriptor() {
} 

void VulkanDescriptor::createDescriptorSetLayout(const VulkanContextInfo& contextInfo) {
	if (VulkanDescriptor::layoutsInitialized == false) 
		initDescriptorSetLayoutTypes(contextInfo);

	descriptorSetLayout = VulkanDescriptor::layoutTypes[numImageSamplers];
}
void VulkanDescriptor::createDescriptorPool(const VulkanContextInfo& contextInfo) {
	std::vector<VkDescriptorPoolSize> poolSizes(numImageSamplers+1);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	for (int i = 1; i < numImageSamplers+1; ++i) {
		poolSizes[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[i].descriptorCount = 1;
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(contextInfo.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create descriptor set pool!";
		throw std::runtime_error(ss.str());
	}
}

//void VulkanDescriptor::createDescriptorSet(const VulkanContextInfo& contextInfo, const VkBuffer& uniformBuffer,
//	const int sizeofUBOstruct, const Mesh* const mesh)
//{
//	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
//	VkDescriptorSetAllocateInfo allocInfo = {};
//	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//	allocInfo.descriptorPool = descriptorPool;
//	allocInfo.descriptorSetCount = 1;
//	allocInfo.pSetLayouts = layouts;
//
//	if (vkAllocateDescriptorSets(contextInfo.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
//		throw std::runtime_error("failed to allocate descriptor set!");
//	}
//
//	VkDescriptorBufferInfo bufferInfo = {};
//	bufferInfo.buffer = uniformBuffer;
//	bufferInfo.offset = 0;
//	bufferInfo.range = sizeofUBOstruct;
//
//	//TODO: make a vector and cycle through Texture vector to determine where they should go
//	std::vector< std::vector<int> > texMapIndices = { mesh->diffuseindices, 
//		mesh->norindices, mesh->specindices, mesh->heightindices };
//	std::vector<VkDescriptorImageInfo> imageInfos(numImageSamplers);
//	for (int i = 0; i < numImageSamplers; ++i) {
//		VkDescriptorImageInfo imageInfo = {};
//		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		if (i == 2 && (type == DescriptorType::HAS_HEIGHT)) {//HAS_HEIGHT implies doesnt have SPEC
//			//just use the first index for each of the maps since it would be odd to have multiple
//			//of a type for a single mesh
//			imageInfo.imageView = mesh->mTextures[texMapIndices[3][0]].vulkanImage.imageView;
//			imageInfo.sampler = mesh->mTextures[texMapIndices[3][0]].vulkanImage.sampler;
//		} else {
//			imageInfo.imageView = mesh->mTextures[texMapIndices[i][0]].vulkanImage.imageView;
//			imageInfo.sampler = mesh->mTextures[texMapIndices[i][0]].vulkanImage.sampler;
//		}
//		imageInfos[i] = imageInfo;
//	}
//
//	//TODO: make vector size of numImageSamplers+1 and cycle through imageInfo above for descriptorWrites[1+]
//	std::vector<VkWriteDescriptorSet> descriptorWrites(numImageSamplers+1);
//	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//	descriptorWrites[0].dstSet = descriptorSet;
//	descriptorWrites[0].dstBinding = 0;
//	descriptorWrites[0].dstArrayElement = 0;
//	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//	descriptorWrites[0].descriptorCount = 1;
//	descriptorWrites[0].pBufferInfo = &bufferInfo;
//
//	for (int i = 1; i < numImageSamplers + 1; ++i) {
//		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//		descriptorWrites[i].dstSet = descriptorSet;
//		descriptorWrites[i].dstBinding = i;
//		descriptorWrites[i].dstArrayElement = 0;
//		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//		descriptorWrites[i].descriptorCount = 1;
//		descriptorWrites[i].pImageInfo = &imageInfos[i-1];
//	}
//
//	vkUpdateDescriptorSets(contextInfo.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
//}
//void VulkanDescriptor::determineDescriptorType(const uint32_t diffuseSize, const uint32_t specSize,
//	const uint32_t norSize, const uint32_t heightSize) {
void VulkanDescriptor::determineDescriptorType(const Mesh* const mesh) {
	const uint32_t diffuseSize = mesh->diffuseindices.size();
	const uint32_t specSize = mesh->specindices.size();
	const uint32_t norSize = mesh->norindices.size();
	const uint32_t heightSize = mesh->heightindices.size();
	if (diffuseSize == 0) {
		type = DescriptorType::HAS_NONE;
		numImageSamplers = 0;
	} else if (diffuseSize > 0 && norSize == 0 && specSize == 0 && heightSize == 0) {
		type = DescriptorType::HAS_DIFFUSE;
		numImageSamplers = 1;
	} else if (diffuseSize > 0 && norSize > 0 && specSize == 0 && heightSize == 0) {
		type = DescriptorType::HAS_NOR;
		numImageSamplers = 2;
	} else if (diffuseSize > 0 && norSize > 0 && specSize > 0 && heightSize == 0) {
		type = DescriptorType::HAS_SPEC;
		numImageSamplers = 3;
	} else if (diffuseSize > 0 && norSize == 0 && specSize == 0 && heightSize > 0) {
		type = DescriptorType::HAS_HEIGHT;
		numImageSamplers = 3;
	} else if (diffuseSize > 0 && norSize  > 0 && specSize  > 0 && heightSize  > 0) {
		type = DescriptorType::HAS_ALL; 
		numImageSamplers = 4;
	} else {
		std::stringstream ss; ss << "\nLINE: " << __LINE__ << ": FILE: " << __FILE__ << ": failed to determine mesh's descriptor type";
		throw std::runtime_error(ss.str());
		type = DescriptorType::HAS_NONE;
		numImageSamplers = 0;
	}
}

void VulkanDescriptor::createDescriptorSet(const VulkanContextInfo& contextInfo, const VkBuffer& uniformBuffer,
	const int sizeofUBOstruct, const Mesh* const mesh)
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(contextInfo.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeofUBOstruct;

	//TODO: make a vector and cycle through Texture vector to determine where they should go
	std::vector< uint32_t > texMapIndices;
	if ((textureMapFlags & HAS_NONE) == HAS_NONE) {
		int adiwinof = 0;
	}
	if ((textureMapFlags & HAS_DIFFUSE) == HAS_DIFFUSE) {
		texMapIndices.push_back(mesh->diffuseindices[0]);
	}
	if ((textureMapFlags & HAS_NOR) == HAS_NOR) {
		texMapIndices.push_back(mesh->norindices[0]);
	}
	if ((textureMapFlags & HAS_HEIGHT) == HAS_HEIGHT) {
		texMapIndices.push_back(mesh->heightindices[0]);
	}
	if ((textureMapFlags & HAS_SPEC) == HAS_SPEC) {
		texMapIndices.push_back(mesh->specindices[0]);
	}
	std::vector<VkDescriptorImageInfo> imageInfos(numImageSamplers);
	for (int i = 0; i < numImageSamplers; ++i) {
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = mesh->mTextures[texMapIndices[i]].vulkanImage.imageView;
		imageInfo.sampler	= mesh->mTextures[texMapIndices[i]].vulkanImage.sampler;
		imageInfos[i] = imageInfo;
	}

	//TODO: make vector size of numImageSamplers+1 and cycle through imageInfo above for descriptorWrites[1+]
	std::vector<VkWriteDescriptorSet> descriptorWrites(numImageSamplers+1);
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	for (int i = 1; i < numImageSamplers + 1; ++i) {
		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i].dstSet = descriptorSet;
		descriptorWrites[i].dstBinding = i;
		descriptorWrites[i].dstArrayElement = 0;
		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[i].descriptorCount = 1;
		descriptorWrites[i].pImageInfo = &imageInfos[i-1];
	}

	vkUpdateDescriptorSets(contextInfo.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VulkanDescriptor::determineNumImageSamplersAndTextureMapFlags(const Mesh* const mesh) {
	if (mesh->diffuseindices.size() == 0) {
		textureMapFlags |= HAS_NONE;
		numImageSamplers = 0;
		return;
	} else {
		textureMapFlags |= HAS_DIFFUSE;
		numImageSamplers = 1;
	} 
	if (mesh->specindices.size() > 0) {
		textureMapFlags |= HAS_SPEC;
		numImageSamplers++;
	}
	if (mesh->norindices.size() > 0) {
		textureMapFlags |= HAS_NOR;
		numImageSamplers++;
	}
	if (mesh->heightindices.size() > 0) {
		textureMapFlags |= HAS_HEIGHT;
		numImageSamplers++;
	}
	//std::stringstream ss; ss << "\nLINE: " << __LINE__ << ": FILE: " << __FILE__ << ": failed to determine mesh's descriptor type";
	//throw std::runtime_error(ss.str());
}


void VulkanDescriptor::destroyVulkanDescriptor(const VulkanContextInfo& contextInfo) {
	destroyDescriptorPool(contextInfo);
	destroyDescriptorSetLayout(contextInfo);
}

void VulkanDescriptor::destroyDescriptorPool(const VulkanContextInfo& contextInfo) {
	vkDestroyDescriptorPool(contextInfo.device, descriptorPool, nullptr);
}

void VulkanDescriptor::destroyDescriptorSetLayout(const VulkanContextInfo& contextInfo) {
	if(descriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(contextInfo.device, descriptorSetLayout, nullptr);
}