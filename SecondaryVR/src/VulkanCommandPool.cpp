#include "VulkanCommandPool.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>


//TODO: pass in a enum to specify for which command queue the pool needs to be made.
VulkanCommandPool::VulkanCommandPool(const VulkanDevices& devices) {
	createGraphicsCommandPool(devices);
}


VulkanCommandPool::~VulkanCommandPool() {
}

void VulkanCommandPool::createGraphicsCommandPool(const VulkanDevices& devices) {
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = devices.graphicsFamily;

	if (vkCreateCommandPool(devices.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create graphics command pool!";
		throw std::runtime_error(ss.str());
	}
}
