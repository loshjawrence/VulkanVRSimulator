#include "VulkanDescriptor.h"

#include <array>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

VulkanDescriptor::VulkanDescriptor() {}

VulkanDescriptor::VulkanDescriptor(const VulkanContextInfo& contextInfo) {
	createDescriptorSetLayout(contextInfo);
	createDescriptorPool(contextInfo);
}

VulkanDescriptor::~VulkanDescriptor() {
} 

void VulkanDescriptor::createDescriptorSetLayout(const VulkanContextInfo& contextInfo) {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.pImmutableSamplers = nullptr;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(contextInfo.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create descriptor set layout!";
		throw std::runtime_error(ss.str());
	}
}
void VulkanDescriptor::createDescriptorPool(const VulkanContextInfo& contextInfo) {
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

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

void VulkanDescriptor::createDescriptorSet(const VulkanContextInfo& contextInfo, const VkBuffer& uniformBuffer,
	const int sizeofUBOstruct, const VkImageView& imageView, const VkSampler& sampler)
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

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(contextInfo.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VulkanDescriptor::destroyVulkanDescriptor(const VulkanContextInfo& contextInfo) {
	destroyDescriptorPool(contextInfo);
	destroyDescriptorSetLayout(contextInfo);
}

void VulkanDescriptor::destroyDescriptorPool(const VulkanContextInfo& contextInfo) {
	vkDestroyDescriptorPool(contextInfo.device, descriptorPool, nullptr);
}

void VulkanDescriptor::destroyDescriptorSetLayout(const VulkanContextInfo& contextInfo) {
	vkDestroyDescriptorSetLayout(contextInfo.device, descriptorSetLayout, nullptr);
}