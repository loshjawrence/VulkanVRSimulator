#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN


#include <string>

//TODO: turn into base image class and subclasses

class VulkanContextInfo;

enum class IMAGETYPE {
	DEPTH=0, TEXTURE, COLOR_ATTACHMENT
};

class VulkanImage {
public:
	VkExtent2D extent;
	VkFormat format;
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory imageMemory;
	IMAGETYPE imagetype;

	//for textures
	std::string filepath;
	VkSampler sampler;

public:
	VulkanImage();
	VulkanImage(const IMAGETYPE& imagetype, const VkExtent2D& extent, const VkFormat& format,
		const VulkanContextInfo& contextInfo, std::string& filepath = std::string(""));
	~VulkanImage();

	void operator=(const VulkanImage& rightside);
	void createColorAttachmentImage(const VulkanContextInfo& contextInfo);
	void createDepthImage(const VulkanContextInfo& contextInfo);
	void createTextureImage(const VulkanContextInfo& contextInfo);
	void createDepthImageWithImportedStaticStencilMask(const VulkanContextInfo& contextInfo);
	void createImage(const VulkanContextInfo& contextInfo);
	void createImageView(const VulkanContextInfo& contextInfo);
	void transitionImageLayout(const VulkanContextInfo& contextInfo,
		const VkImageLayout oldLayout, const VkImageLayout newLayout, bool fillStencil = false);
	void createImageSampler(const VulkanContextInfo& contextInfo);

	static VkImageView createImageView(const VkImage& image, const VkFormat& format,
		const VkImageAspectFlags& aspectFlags, const VkDevice& device);


	//helper
	void covertToSingleByte(uint8_t* stencilBytes, const uint8_t* pixels,
		const int width, const int height, const int channelsInPixels);

	//cleanup
	void destroyVulkanImage(const VulkanContextInfo& contextInfo);
	void destroySampler(const VulkanContextInfo& contextInfo);
	void destroyImageView(const VulkanContextInfo& contextInfo);
	void destroyImage(const VulkanContextInfo& contextInfo);
	void destroyImageMemory(const VulkanContextInfo& contextInfo);
};

