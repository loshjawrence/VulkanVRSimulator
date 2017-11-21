#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN


#include "VulkanContextInfo.h"
#include <string>

//TODO: turn into base image class and subclasses

enum class IMAGETYPE {
	DEPTH=0, TEXTURE=1,
};

class VulkanImage {
public:
	VkExtent2D extent;
	VkFormat format;
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory imageMemory;
	const IMAGETYPE imagetype;

	//for textures
	std::string filepath;
	VkSampler sampler;

public:
	VulkanImage(const IMAGETYPE& imagetype, const VkExtent2D& extent, const VkFormat& format,
		const VulkanContextInfo& contextInfo, const VkCommandPool& commandPool, std::string& filepath = std::string(""));
	~VulkanImage();

	void createDepthImage(const VulkanContextInfo& contextInfo, const VkCommandPool& commandPool);
	void createTextureImage(const VulkanContextInfo& contextInfo, const VkCommandPool& commandPool);
	void createImage(const VulkanContextInfo& contextInfo);
	void createImageView(const VulkanContextInfo& contextInfo);
	void transitionImageLayout(const VulkanContextInfo& contextInfo, const VkCommandPool& commandPool,
		const VkImageLayout oldLayout, const VkImageLayout newLayout);
	void createImageSampler(const VulkanContextInfo& contextInfo);

	static VkImageView createImageView(const VkImage& image, const VkFormat& format,
		const VkImageAspectFlags& aspectFlags, const VkDevice& device);
};

