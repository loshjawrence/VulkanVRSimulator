#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "VulkanDevices.h"
#include <string>

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
	std::string filepath;

public:
	VulkanImage(const IMAGETYPE& imagetype, const VkExtent2D& extent, const VkFormat& format,
		const VulkanDevices& devices, const VkCommandPool& commandPool, std::string& filepath = std::string(""));
	~VulkanImage();

	void createImage(const VulkanDevices& devices);
	void createImageView(const VulkanDevices& devices);
	void transitionImageLayout(const VulkanDevices& devices, const VkCommandPool& commandPool);
};

