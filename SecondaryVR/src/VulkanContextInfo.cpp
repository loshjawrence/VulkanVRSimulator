#pragma once
#include "VulkanContextInfo.h"
#include "Camera.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <algorithm>
#include <array>


VulkanContextInfo::VulkanContextInfo() {

}
VulkanContextInfo::VulkanContextInfo(GLFWwindow* window)
{
	createInstance();
	createSurface(window);
	pickPhysicalDevice();
	createLogicalDevice();
	acquireDeviceQueues();
	addGraphicsCommandPool(1);
	createSwapChain(window);
	createSwapChainImageViews();
	determineDepthFormat();
	camera = Camera();
	initStencils();
	createDepthImage();
}


VulkanContextInfo::~VulkanContextInfo() {
}

void VulkanContextInfo::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": validation layers requested, but none available!";
		throw std::runtime_error(ss.str());
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create instance!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanContextInfo::createSurface(GLFWwindow* window) {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create surface!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanContextInfo::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": no physical devices!";
		throw std::runtime_error(ss.str());
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to find suitable GPU!";
		throw std::runtime_error(ss.str());
	}
}

bool VulkanContextInfo::isDeviceSuitable(const VkPhysicalDevice& device) {
	determineQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		determinePhysicalDeviceSurfaceDetails(device);
		swapChainAdequate = !surfaceDetails.formats.empty() && !surfaceDetails.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return hasGraphicsAndPresentQueueFamilies && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.fillModeNonSolid;
}

void VulkanContextInfo::determineQueueFamilies(const VkPhysicalDevice& device) {
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport) {
			presentFamily = i;
		}

		if (graphicsFamily > -1 && presentFamily > -1) {//got all the queues we need
			hasGraphicsAndPresentQueueFamilies = true;
			break;
		}

		i++;
	}
}

bool VulkanContextInfo::checkDeviceExtensionSupport(const VkPhysicalDevice& device) const {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void VulkanContextInfo::determinePhysicalDeviceSurfaceDetails(const VkPhysicalDevice& device) {

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceDetails.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		surfaceDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, surfaceDetails.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		surfaceDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, surfaceDetails.presentModes.data());
	}
}

void VulkanContextInfo::createLogicalDevice() {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { graphicsFamily, presentFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create logical device!";
		throw std::runtime_error(ss.str());
	}

}

void VulkanContextInfo::acquireDeviceQueues() {
	vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
}

void VulkanContextInfo::initStencils() {
	radialDensityMasks.resize(camera.numQualitySettings);
	for (int i = 0; i < camera.numQualitySettings; ++i) {
		radialDensityMasks[i]			= PreMadeStencil(*this,i, StencilType::RadialDensityMask);
	}
}

void VulkanContextInfo::createDepthImage() {
	determineDepthFormat();
	if (!camera.vrmode || (camera.vrmode && camera.timewarp)) {
		depthImage = VulkanImage(IMAGETYPE::DEPTH, camera.renderTargetExtent, depthFormat, *this, std::string(""));
	} else {
		const int i = camera.qualityIndex;
		depthImage = VulkanImage(IMAGETYPE::DEPTH, camera.renderTargetExtent, depthFormat, *this, radialDensityMasks[i].filename);
	}
}

void VulkanContextInfo::determineDepthFormat() {
	const std::vector<VkFormat> stencilformat = { VK_FORMAT_D32_SFLOAT_S8_UINT };
	const std::vector<VkFormat> defaultformats = { VK_FORMAT_D32_SFLOAT,VK_FORMAT_D32_SFLOAT_S8_UINT,VK_FORMAT_D24_UNORM_S8_UINT };
	depthFormat = findSupportedFormat(
	camera.useStencil ? stencilformat : defaultformats,
		VK_IMAGE_TILING_OPTIMAL,
		//VK_IMAGE_TILING_LINEAR,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT 
	);
}

VkFormat VulkanContextInfo::findSupportedFormat(const std::vector<VkFormat>& candidates, 
	const VkImageTiling tiling, const VkFormatFeatureFlags features) const 
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to find supported depth format for physical device!";
	throw std::runtime_error(ss.str());
}
bool VulkanContextInfo::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}


std::vector<const char*> VulkanContextInfo::getRequiredExtensions() {
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

void VulkanContextInfo::addGraphicsCommandPool(const int num) {
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	for (int i = 0; i < num; ++i) {
		VkCommandPool commandPool;
		graphicsCommandPools.push_back(commandPool);

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &graphicsCommandPools.back()) != VK_SUCCESS) {
			std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create graphics command pool!";
			throw std::runtime_error(ss.str());
		}
	}
}

void VulkanContextInfo::createSwapChain(GLFWwindow* window) {
	determinePhysicalDeviceSurfaceDetails(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(surfaceDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(surfaceDetails.presentModes);
	VkExtent2D extent = chooseSwapExtent(surfaceDetails.capabilities, window);

	uint32_t imageCount = surfaceDetails.capabilities.minImageCount + 1;
	if (surfaceDetails.capabilities.maxImageCount > 0 && imageCount > surfaceDetails.capabilities.maxImageCount) {
		imageCount = surfaceDetails.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	if (graphicsFamily != presentFamily) {
		uint32_t queueFamilyIndices[] = { (uint32_t)graphicsFamily, (uint32_t)presentFamily };
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = surfaceDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create swapchain!";
		throw std::runtime_error(ss.str());
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

VkSurfaceFormatKHR VulkanContextInfo::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanContextInfo::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		} else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D VulkanContextInfo::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

void VulkanContextInfo::createSwapChainImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = VulkanImage::createImageView(swapChainImages[i], swapChainImageFormat, 
												VK_IMAGE_ASPECT_COLOR_BIT, device);
	}
}

void VulkanContextInfo::createSwapChainFramebuffers(const VkRenderPass& renderPass)
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		//std::vector<VkImageView> attachments = { swapChainImageViews[i], depthImage.imageView };
		std::vector<VkImageView> attachments = { swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create framebuffer!";
			throw std::runtime_error(ss.str());
		}
	}
}

void VulkanContextInfo::destroyVulkanSwapChain() {
	destroySwapChainFramebuffers();
	destroySwapChainImageViews();
	destroySwapChain();
}

void VulkanContextInfo::destroySwapChainFramebuffers() {
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
	    vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}
}

void VulkanContextInfo::destroySwapChainImageViews() {
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
}

void VulkanContextInfo::destroySwapChain() {
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VulkanContextInfo::destroyCommandPools() {
	for (int i = 0; i < graphicsCommandPools.size(); ++i) {
		vkDestroyCommandPool(device, graphicsCommandPools[i], nullptr);
	}
	for (int i = 0; i < computeCommandPools.size(); ++i) {
		vkDestroyCommandPool(device, computeCommandPools[i], nullptr);
	}
}

void VulkanContextInfo::destroyDevice() {
	vkDestroyDevice(device, nullptr);
}

void VulkanContextInfo::destroySurface() {
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

void VulkanContextInfo::destroyInstance() {
	vkDestroyInstance(instance, nullptr);
}

