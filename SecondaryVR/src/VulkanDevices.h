#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "GlobalSettings.h"
#include <vector>


struct PhysicalDeviceSurfaceDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevices {
public:
    VkDevice device;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

    VkSurfaceKHR surface;

    int graphicsFamily = -1;
    int presentFamily = -1;
	bool hasGraphicsAndPresentQueueFamilies = false;

	PhysicalDeviceSurfaceDetails surfaceDetails;

	const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkFormat depthFormat;
public:

	//TODO: make some of these private
	VulkanDevices(const VkInstance& instance, GLFWwindow* window);
	~VulkanDevices();
	void createSurface(const VkInstance& instance, GLFWwindow* window);
	void pickPhysicalDevice(const VkInstance& instance);
	bool isDeviceSuitable(const VkPhysicalDevice& device);
	void determineQueueFamilies(const VkPhysicalDevice& device);
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device) const;
	void determinePhysicalDeviceSurfaceDetails(const VkPhysicalDevice& device);
	void createLogicalDevice();
	void determineDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
		const VkImageTiling tiling, const VkFormatFeatureFlags features) const;
};

