#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "Vertex.h"
class VulkanContexInfo;


//VkBuffer buffers are required in various places, just bundle the methods into a static class (namespaced functions)
class VulkanBuffer {
public:
	static void createBuffer(const VulkanContextInfo& contextInfo, const VkDeviceSize& size, const VkBufferUsageFlags& usage, 
		const VkMemoryPropertyFlags& properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	static void copyBuffer(const VulkanContextInfo& contextInfo, const VkCommandPool& commandPool, VkBuffer& srcBuf,
		VkBuffer& dstBuf, const VkDeviceSize& size);

	static void createUniformBuffer(const VulkanContextInfo& contextInfo, const VkDeviceSize& bufferSize,
		VkBuffer& uniformBuffer, VkDeviceMemory& uniformBufferMemory);

	static void createVertexBuffer(const VulkanContextInfo& contextInfo, const VkCommandPool& commandPool, 
		const std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);

	static void createIndexBuffer(const VulkanContextInfo& contextInfo, const VkCommandPool& commandPool, 
		const std::vector<uint32_t>& indices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);

private:
	//disallow creation of an instance of this class
	VulkanBuffer();
	~VulkanBuffer();
};

