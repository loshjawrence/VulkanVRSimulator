#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "VulkanDevices.h"

//a layout is essentially nothing more that a struct defintion telling the driver
//the data types that are going to used in a set

//MAYBE: this class can be a container for the different layouts that we'll need across the program
//OR: maybe pass an enum to specify what kind of predefined layout to make, and have a container somewhere else

class VulkanDescriptorSetLayout {
public:
	VkDescriptorSetLayout descriptorSetLayout;
public:
	VulkanDescriptorSetLayout(const VulkanDevices& devices);
	~VulkanDescriptorSetLayout();
	void createDescriptorSetLayout(const VulkanDevices& devices);
};

