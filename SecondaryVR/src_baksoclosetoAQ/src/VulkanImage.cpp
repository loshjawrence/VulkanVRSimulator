#pragma once
#include "VulkanImage.h"
#include "Utils.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>


#ifndef STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION YEA DONT ADD THIS AS WELL, the define needs to be in only ONE c or cpp file
#include <stb_image.h>
#endif

//helper
bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VulkanImage::VulkanImage() {

}

VulkanImage::VulkanImage(const IMAGETYPE& imagetype, const VkExtent2D& extent, const VkFormat& format,
	const VulkanContextInfo& contextInfo, std::string& filepath)
	: extent(extent), format(format), imagetype(imagetype), filepath(filepath)
{
	if (imagetype == IMAGETYPE::DEPTH) {
		createDepthImage(contextInfo);
	} else if (imagetype == IMAGETYPE::TEXTURE) {
		createTextureImage(contextInfo);
	} else if (imagetype == IMAGETYPE::COLOR_ATTACHMENT) {
		createColorAttachmentImage(contextInfo);
		//sceneImageStage->createImages(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
}


VulkanImage::~VulkanImage() {
}

void VulkanImage::operator=(const VulkanImage& rightside) {
	extent			= rightside.extent;
	format 			= rightside.format;
	image			= rightside.image;
	imageView		= rightside.imageView;
	imageMemory		= rightside.imageMemory;
	imagetype		= rightside.imagetype;
	filepath		= rightside.filepath;
	sampler			= rightside.sampler;

	//no need for cascading assigment so no need to return *this
}

void VulkanImage::createColorAttachmentImage(const VulkanContextInfo& contextInfo) {
	createImage(contextInfo);
	createImageView(contextInfo);
	createImageSampler(contextInfo);
}

void VulkanImage::createDepthImage(const VulkanContextInfo& contextInfo) {
	if (filepath != std::string("")) {
		createDepthImageWithImportedStaticStencilMask(contextInfo);
	} else {
		createImage(contextInfo);
		createImageView(contextInfo);
		transitionImageLayout(contextInfo, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}
}
void VulkanImage::createDepthImageWithImportedStaticStencilMask(const VulkanContextInfo& contextInfo) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	extent.width = texWidth;
	extent.height = texHeight;
	VkDeviceSize imageSize = extent.width * extent.height * 1;


	if (!pixels) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to load texture image!";
		throw std::runtime_error(ss.str());
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(contextInfo, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	//convert to single bytes and test print to see if image got through ok
	uint8_t* stencilBytes = new uint8_t[extent.width*extent.height];
	for (int index = 0; index < extent.width*extent.height; ++index) {
		stencilBytes[index] = pixels[index * 4];
	}



	void* data;
	vkMapMemory(contextInfo.device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, stencilBytes, static_cast<size_t>(imageSize));
	vkUnmapMemory(contextInfo.device, stagingBufferMemory);

	stbi_image_free(pixels);
	free(stencilBytes);
	createImage(contextInfo);

	transitionImageLayout(contextInfo, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, true);
	copyBufferToImage(contextInfo, stagingBuffer, image, extent.width, extent.height, VK_IMAGE_ASPECT_STENCIL_BIT);//spec says that only one(depth or stencil aspect) can be written to at a time
	transitionImageLayout(contextInfo, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	vkDestroyBuffer(contextInfo.device, stagingBuffer, nullptr);
	vkFreeMemory(contextInfo.device, stagingBufferMemory, nullptr);

	createImageView(contextInfo);
}


void VulkanImage::createTextureImage(const VulkanContextInfo& contextInfo) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	extent.width = texWidth;
	extent.height = texHeight;
	VkDeviceSize imageSize = extent.width * extent.height * 4;

	if (!pixels) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to load texture image!";
		throw std::runtime_error(ss.str());
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(contextInfo, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(contextInfo.device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(contextInfo.device, stagingBufferMemory);

	stbi_image_free(pixels);
	createImage(contextInfo);

	transitionImageLayout(contextInfo, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(contextInfo, stagingBuffer, image, extent.width, extent.height, VK_IMAGE_ASPECT_COLOR_BIT);
	transitionImageLayout(contextInfo, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(contextInfo.device, stagingBuffer, nullptr);
	vkFreeMemory(contextInfo.device, stagingBufferMemory, nullptr);

	createImageView(contextInfo);

	createImageSampler(contextInfo);
}

void VulkanImage::createImage(const VulkanContextInfo& contextInfo) {
	//VkThings
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags properties;
	if (imagetype == IMAGETYPE::DEPTH) {
		tiling = VK_IMAGE_TILING_OPTIMAL;
		if (filepath != std::string("")) {
			usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		} else {
			usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	} else if (imagetype == IMAGETYPE::TEXTURE) {
		tiling = VK_IMAGE_TILING_OPTIMAL;
		usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	} else if (imagetype == IMAGETYPE::COLOR_ATTACHMENT) {
		tiling = VK_IMAGE_TILING_OPTIMAL;
		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
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

	if (vkCreateImage(contextInfo.device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create image!";
		throw std::runtime_error(ss.str());
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(contextInfo.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(contextInfo.physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(contextInfo.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to allocate image memory!";
		throw std::runtime_error(ss.str());
	}

	vkBindImageMemory(contextInfo.device, image, imageMemory, 0);
}

void VulkanImage::createImageView(const VulkanContextInfo& contextInfo) {
	VkImageAspectFlags aspectFlags;
	if (imagetype == IMAGETYPE::DEPTH) {
		if (hasStencilComponent(format)) {
			aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		} else {
			aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
	} else if (imagetype == IMAGETYPE::TEXTURE || imagetype == IMAGETYPE::COLOR_ATTACHMENT) {
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
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

	if (vkCreateImageView(contextInfo.device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create image view!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanImage::transitionImageLayout(const VulkanContextInfo& contextInfo, 
	const VkImageLayout oldLayout, const VkImageLayout newLayout, bool fillStencil) 
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(contextInfo);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || fillStencil) {
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
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

	endSingleTimeCommands(contextInfo, commandBuffer);
}
void VulkanImage::createImageSampler(const VulkanContextInfo& contextInfo) {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	if (imagetype == IMAGETYPE::COLOR_ATTACHMENT) {
		samplerInfo.magFilter - VK_FILTER_NEAREST;
		samplerInfo.minFilter - VK_FILTER_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
	} else if (imagetype == IMAGETYPE::TEXTURE) {
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 4;
	} else {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": tried to create sampler for image of unknown type!";
		throw std::runtime_error(ss.str());
	}

	if (vkCreateSampler(contextInfo.device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create texture sampler!";
		throw std::runtime_error(ss.str());
	}

}
VkImageView VulkanImage::createImageView(const VkImage& image, const VkFormat& format,
		const VkImageAspectFlags& aspectFlags, const VkDevice& device)
{
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

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create image view!";
		throw std::runtime_error(ss.str());
	}
	return imageView;
}

void VulkanImage::destroyVulkanImage(const VulkanContextInfo& contextInfo) {
	if (imagetype == IMAGETYPE::TEXTURE) {
		destroySampler(contextInfo);
	}
	destroyImageView(contextInfo);
	destroyImage(contextInfo);
	destroyImageMemory(contextInfo);
}

void VulkanImage::destroySampler(const VulkanContextInfo& contextInfo) {
	if(sampler != VK_NULL_HANDLE)
		vkDestroySampler(contextInfo.device, sampler, nullptr);
}

void VulkanImage::destroyImageView(const VulkanContextInfo& contextInfo) {
	if(imageView != VK_NULL_HANDLE)
		vkDestroyImageView(contextInfo.device, imageView, nullptr);
}

void VulkanImage::destroyImage(const VulkanContextInfo& contextInfo) {
	if(image != VK_NULL_HANDLE)
		vkDestroyImage(contextInfo.device, image, nullptr);
}

void VulkanImage::destroyImageMemory(const VulkanContextInfo& contextInfo) {
	if(imageMemory != VK_NULL_HANDLE)
		vkFreeMemory(contextInfo.device, imageMemory, nullptr);
}
