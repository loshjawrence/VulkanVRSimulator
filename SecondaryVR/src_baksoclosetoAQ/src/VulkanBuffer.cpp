#pragma once
#include "VulkanBuffer.h"

#include "Utils.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>


VulkanBuffer::VulkanBuffer() {
}


VulkanBuffer::~VulkanBuffer() {
}

void VulkanBuffer::createBuffer(const VulkanContextInfo& contextInfo, const VkDeviceSize& size, const VkBufferUsageFlags& usage, 
	const VkMemoryPropertyFlags& properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) 
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(contextInfo.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create buffer!";
		throw std::runtime_error(ss.str());
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(contextInfo.device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(contextInfo.physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(contextInfo.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to alloc buffer memory!";
		throw std::runtime_error(ss.str());
	}

	vkBindBufferMemory(contextInfo.device, buffer, bufferMemory, 0);
}

void VulkanBuffer::copyBuffer(const VulkanContextInfo& contextInfo,
	VkBuffer& srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize& size) 
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(contextInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(contextInfo, commandBuffer);
}

void VulkanBuffer::createUniformBuffer(const VulkanContextInfo& contextInfo, const VkDeviceSize& bufferSize, 
	VkBuffer& uniformBuffer, VkDeviceMemory& uniformBufferMemory) 
{
	createBuffer(contextInfo, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
}

void VulkanBuffer::createVertexBuffer(const VulkanContextInfo& contextInfo,
	const std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory) 
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(contextInfo, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(contextInfo.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(contextInfo.device, stagingBufferMemory);

	createBuffer(contextInfo, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	copyBuffer(contextInfo, stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(contextInfo.device, stagingBuffer, nullptr);
	vkFreeMemory(contextInfo.device, stagingBufferMemory, nullptr);
}

void VulkanBuffer::createIndexBuffer(const VulkanContextInfo& contextInfo, 
	const std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory)
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(contextInfo, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(contextInfo.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(contextInfo.device, stagingBufferMemory);

	createBuffer(contextInfo, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(contextInfo, stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(contextInfo.device, stagingBuffer, nullptr);
	vkFreeMemory(contextInfo.device, stagingBufferMemory, nullptr);
}
