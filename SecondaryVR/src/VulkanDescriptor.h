#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "VulkanContextInfo.h"


//TODO: make base and subclasses

class VulkanDescriptor {
public:
	//a layout is like a struct defintion telling the driver what data types are used in a set
	VkDescriptorSetLayout descriptorSetLayout;

	//pool is sized to the layout to prevent fragmentation in the pool
	VkDescriptorPool descriptorPool;

	//set is that actual handle to the data that we'll update when one of its members change
	VkDescriptorSet descriptorSet;

	//VkBuffer for the UBO
	VkBuffer UBO;

	//sizeof the UBO struct
	int sizeofUBOstruct;

	//image view and sampler for the combined sampler image
	VkImageView imageView;
	VkSampler sampler;



public:
	VulkanDescriptor(const VulkanContextInfo& contextInfo);
	~VulkanDescriptor();
	void createDescriptorSetLayout(const VulkanContextInfo& contextInfo);
	void createDescriptorPool(const VulkanContextInfo& contextInfo);
	void createDescriptorSet(const VulkanContextInfo& contextInfo, const VkBuffer& UBO,
		const int sizeofUBOstruct, const VkImageView& imageView, const VkSampler& sampler);
};

