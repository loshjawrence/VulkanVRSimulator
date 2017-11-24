#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN



#include "VulkanContextInfo.h"
#include <vector>


//TODO: make base and subclasses

enum class DescriptorType {
	HAS_NONE = 0, HAS_DIFFUSE, HAS_NOR, HAS_SPEC, HAS_HEIGHT, HAS_ALL
};
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

	DescriptorType type = DescriptorType::HAS_DIFFUSE; 
	int numImageSamplers = 1;

	static const int MAX_IMAGESAMPLERS = 4;
	static bool layoutsInitialized;
	static std::vector<VkDescriptorSetLayout> layoutTypes;

public:
	VulkanDescriptor();
	VulkanDescriptor(const VulkanContextInfo& contextInfo);
	~VulkanDescriptor();
	void createDescriptorSetLayout(const VulkanContextInfo& contextInfo);
	void createDescriptorPool(const VulkanContextInfo& contextInfo);
//	void createDescriptorSet(const VulkanContextInfo& contextInfo, const VkBuffer& UBO,
//		const int sizeofUBOstruct, const VulkanImage& vulkanImage);
	void createDescriptorSet(const VulkanContextInfo& contextInfo, const VkBuffer& UBO,
		const int sizeofUBOstruct, const Mesh* const mesh);

	//void determineDescriptorType(const uint32_t diffuseSize, const uint32_t specSize, 
	//	const uint32_t norSize, const uint32_t heightSize);
	void determineDescriptorType(const Mesh* const mesh);

	void destroyVulkanDescriptor(const VulkanContextInfo& contextInfo);
	void destroyDescriptorPool(const VulkanContextInfo& contextInfo);
	void destroyDescriptorSetLayout(const VulkanContextInfo& contextInfo);
};

