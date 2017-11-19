#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "VulkanDevices.h"

class VulkanCommandPool {
public:
	VkCommandPool commandPool;

public:
	VulkanCommandPool(const VulkanDevices& devices);
	~VulkanCommandPool();

	void createGraphicsCommandPool(const VulkanDevices& devices);
};

