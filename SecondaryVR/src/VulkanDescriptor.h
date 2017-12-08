#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN



#include "VulkanContextInfo.h"
#include <vector>


//TODO: make base and subclasses
class Mesh;
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

	static const int MAX_IMAGESAMPLERS	= 4;
	uint32_t textureMapFlags = 0;
	int numImageSamplers = 0;

	static bool layoutsInitialized;
	static std::vector<VkDescriptorSetLayout> layoutTypes;
	static std::vector<VkDescriptorSetLayout> postProcessLayoutTypes;
	static std::vector<VkDescriptorSetLayout> timeWarpLayoutTypes;

public:
	VulkanDescriptor();
	VulkanDescriptor(const VulkanContextInfo& contextInfo);
	~VulkanDescriptor();
	//for drawing descriptors
	void createDescriptorSetLayout(const VulkanContextInfo& contextInfo);
	void createDescriptorPool(const VulkanContextInfo& contextInfo);
	void createDescriptorSet(const VulkanContextInfo& contextInfo, const VkBuffer& UBO,
		const int sizeofUBOstruct, const Mesh* const mesh);

	//NEW
	void createDescriptorSetLayoutPostProcess(const VulkanContextInfo& contextInfo);
	void createDescriptorSetLayoutPostProcessTimeWarp(const VulkanContextInfo& contextInfo);
	void createDescriptorPoolPostProcess(const VulkanContextInfo& contextInfo);
	void createDescriptorPoolPostProcessTimeWarp(const VulkanContextInfo& contextInfo);
	void createDescriptorSetPostProcess(const VulkanContextInfo& contextInfo,
		const std::vector<VulkanImage>& vulkanImages);
	//void createDescriptorSetPostProcessTimeWarp(const VulkanContextInfo& contextInfo,
	//	const std::vector<VulkanImage>& vulkanImages, const VulkanImage& depthImage);
	void createDescriptorSetPostProcessTimeWarp(const VulkanContextInfo& contextInfo,
		const std::vector<VulkanImage>& vulkanImages, const VulkanImage& depthImage, const VkBuffer& uniformBuffer,
		const int sizeofUBOstruct);

	void determineNumImageSamplersAndTextureMapFlags(const Mesh* const mesh);

	void destroyVulkanDescriptor(const VulkanContextInfo& contextInfo);
	void destroyDescriptorPool(const VulkanContextInfo& contextInfo);
	void destroyDescriptorSetLayout(const VulkanContextInfo& contextInfo);
};

