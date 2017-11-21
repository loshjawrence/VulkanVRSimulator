#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "VulkanContextInfo.h"
#include "VulkanDescriptor.h"
#include "VulkanRenderPass.h"
#include "Vertex.h"

#include <vector>
#include <string>

//pipelines describe the configuration of fixed function stages and which shader programs are used with it
//also the layout of descriptor sets (shader resources) that will be used with its shaders
//pipelines get associated with subpasses in a renderpass.
//a renderpass is just a collection of subpasses describing how those subpasses relate to eachother(hand-off results from one subpass to another)


//TODO: make base class and subclasses

class VulkanGraphicsPipeline {
public:
	const std::vector<std::string>& shaderpaths;
	VkPipeline graphicsPipeline;
	VkPipeline pipelineLayout;

public:
	VulkanGraphicsPipeline(const std::vector<std::string>& shaderspaths, const VulkanRenderPass& renderPass,
		const VulkanContextInfo& contextInfo, const VkDescriptorSetLayout* setLayouts);

	~VulkanGraphicsPipeline();

	void createGraphicsPipeline(const VulkanRenderPass& renderPass, const VulkanContextInfo& contextInfo, 
		const VkDescriptorSetLayout* setLayouts);

	VkShaderModule createShaderModule(const std::vector<char>& code, const VulkanContextInfo& contextInfo) const;
};

