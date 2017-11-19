#include "VulkanImage.h"
#include "Utils.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

//helper
bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VulkanImage::VulkanImage(const IMAGETYPE& imagetype, const VkExtent2D& extent, const VkFormat& format,
	const VulkanDevices& devices, const VkCommandPool& commandPool, std::string& filepath)
	: extent(extent), format(format), imagetype(imagetype), filepath(filepath)
{
	createImage(devices);
	createImageView(devices);
	//if needed transition image layout, TODO: look up what this is doing
	transitionImageLayout(devices, commandPool);
}


VulkanImage::~VulkanImage() {
}


void VulkanImage::createImage(const VulkanDevices& devices) {
	//VkThings
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags properties;
	if (imagetype == IMAGETYPE::DEPTH) {
		tiling = VK_IMAGE_TILING_OPTIMAL;
		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	} else if (imagetype == IMAGETYPE::TEXTURE) {

	}

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(devices.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create image!";
		throw std::runtime_error(ss.str());
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(devices.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(devices.physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(devices.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to allocate image memory!";
		throw std::runtime_error(ss.str());
	}

	vkBindImageMemory(devices.device, image, imageMemory, 0);
}

void VulkanImage::createImageView(const VulkanDevices& devices) {
	VkImageAspectFlags aspectFlags;
	if (imagetype == IMAGETYPE::DEPTH) {
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else if (imagetype == IMAGETYPE::TEXTURE) {

	}

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(devices.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create image view!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanImage::transitionImageLayout(const VulkanDevices& devices, const VkCommandPool& commandPool) {
	//VkThings
	VkImageLayout oldLayout; VkImageLayout newLayout;
	if (imagetype == IMAGETYPE::DEPTH) {
		oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	} else if (imagetype == IMAGETYPE::TEXTURE) {

	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(devices.device, commandPool);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": unsupported layout transition!";
		throw std::runtime_error(ss.str());
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(devices.device, commandPool, commandBuffer, devices.graphicsQueue);
}
