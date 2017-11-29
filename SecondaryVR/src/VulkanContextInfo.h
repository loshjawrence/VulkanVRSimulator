#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "GlobalSettings.h"
#include <vector>
#include "VulkanImage.h"
#include "Camera.h"

//This class holds vulkan things that get created once and are used for the duration of the program
//these things generally won't change across typical vulkan applications

struct PhysicalDeviceSurfaceDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


class VulkanContextInfo {
public:
	//devices
    VkDevice device;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	//commandPools
	//generally only need 1, but if you want to do multithreaded command recording each thread needs its own pool
	std::vector<VkCommandPool> graphicsCommandPools;
	std::vector<VkCommandPool> computeCommandPools;

	//queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	//queue indices
    int graphicsFamily = -1;
    int presentFamily = -1;
	bool hasGraphicsAndPresentQueueFamilies = false;

	//surface
    VkSurfaceKHR surface;
	PhysicalDeviceSurfaceDetails surfaceDetails;

	//depth image 
	VkFormat depthFormat;
	VulkanImage depthImage;

	//Camera
	Camera camera;

	//instance
	VkInstance instance;

	//swap chain (screen presentation)
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

private:
	std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

public:

	//TODO: make some of these private
	VulkanContextInfo();
	VulkanContextInfo(GLFWwindow* window);
	~VulkanContextInfo();

	//surface
	void createSurface(GLFWwindow* window);

	//physical device
	void pickPhysicalDevice();
	bool isDeviceSuitable(const VkPhysicalDevice& device);
	void determineQueueFamilies(const VkPhysicalDevice& device);
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device) const;
	void determinePhysicalDeviceSurfaceDetails(const VkPhysicalDevice& device);

	//logical device
	void createLogicalDevice();

	//device command queues
	void acquireDeviceQueues();
	
	//depthstencil format determination
	void createDepthImage();
	void determineDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, 
		const VkImageTiling tiling, const VkFormatFeatureFlags features) const;

	//commandPool creation
	void addGraphicsCommandPool(const int num);

	//instance and helper functions
	void createInstance();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();

	//swap chain 
	void createSwapChain(GLFWwindow* window);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilitie, GLFWwindow* window);
	void createSwapChainImageViews();
	void createSwapChainFramebuffers(const VkRenderPass& renderPass);


	//cleanup
	void destroyVulkanSwapChain();
	void destroySwapChainFramebuffers();
	void destroySwapChainImageViews();
	void destroySwapChain();
	void destroyCommandPools();
	void destroyDevice();
	void destroySurface();
	void destroyInstance();
};

