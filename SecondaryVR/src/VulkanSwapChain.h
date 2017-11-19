#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "VulkanDevices.h"

class VulkanSwapChain {
public:
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
public:
	VulkanSwapChain(const VulkanDevices& devices, GLFWwindow* window);
	~VulkanSwapChain();
	void createSwapChain(const VulkanDevices& device, GLFWwindow* window);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilitie, GLFWwindow* window);
	void createSwapChainImageViews(const VulkanDevices& devices);
	void createFramebuffers(const VkRenderPass& renderPass);
	void createFramebuffers(const VkDevice& device, const VkImageView& depthImageView, const VkRenderPass& renderPass);
};
