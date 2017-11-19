#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN
#include <string>
#include <vector>

class VulkanInstance {
public:
	VkInstance instance;
public:
	VulkanInstance();
	~VulkanInstance();

	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
};

