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
//each material (different set of shaders) will need an instance of this pipeline
//make a post process class since it's setup will be different than a forward render pass

class VulkanGraphicsPipeline {
public:
	std::vector<std::string> shaderpaths;
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;


public:
	VulkanGraphicsPipeline();
	VulkanGraphicsPipeline(const std::vector<std::string>& shaderspaths, const VulkanRenderPass& renderPass,
		const VulkanContextInfo& contextInfo, const VkDescriptorSetLayout* setLayouts);

	~VulkanGraphicsPipeline();

	void createGraphicsPipeline(const VulkanRenderPass& renderPass, const VulkanContextInfo& contextInfo, 
		const VkDescriptorSetLayout* setLayouts);

	VkShaderModule createShaderModule(const std::vector<char>& code, const VulkanContextInfo& contextInfo) const;

	void VulkanGraphicsPipeline::createCommandBuffers(const VulkanContextInfo& contextInfo,
		const VulkanRenderPass& renderPass, const VkBuffer& vertexBuffer, const VkBuffer& indexBuffer,
		const std::vector<uint32_t>& indices, const VulkanDescriptor& descriptor);

	void createSemaphores(const VulkanContextInfo& contextInfo);

	//cleanup
	void freeCommandBuffers(const VulkanContextInfo& contextInfo);
	void destroyPipeline(const VulkanContextInfo& contextInfo);
	void destroyPipelineLayout(const VulkanContextInfo& contextInfo);
	void destroyPipelineSemaphores(const VulkanContextInfo& contextInfo);
	void destroyVulkanPipeline(const VulkanContextInfo& contextInfo);
};

