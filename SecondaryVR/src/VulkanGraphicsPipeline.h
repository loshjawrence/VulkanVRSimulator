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

class Mesh;
class Model;

struct PushConstant {
	glm::mat4 modelMatrix;
	uint32_t toggleFlags;
	static const VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT;
};

class VulkanGraphicsPipeline {
public:
	std::vector<std::string> shaderpaths;
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	//recording state
	bool recording = false;

public:
	VulkanGraphicsPipeline();
	VulkanGraphicsPipeline(const std::vector<std::string>& shaderspaths, const VulkanRenderPass& renderPass,
		const VulkanContextInfo& contextInfo, const VkDescriptorSetLayout* setLayouts);

	~VulkanGraphicsPipeline();

	void allocateCommandBuffers(const VulkanContextInfo& contextInfo);

	void createGraphicsPipeline(const VulkanRenderPass& renderPass, const VulkanContextInfo& contextInfo, 
		const VkDescriptorSetLayout* setLayouts);

	VkShaderModule createShaderModule(const std::vector<char>& code, const VulkanContextInfo& contextInfo) const;

	////SECONDARY RECORDING////
	void VulkanGraphicsPipeline::recordCommandBufferSecondary(const VkCommandBufferInheritanceInfo& inheritanceInfo,
		const uint32_t imageIndex, const VulkanContextInfo& contextInfo,
		const VulkanRenderPass& renderPass, const Model& model, const Mesh& mesh, const bool vrmode);
	void beginRecordingSecondary(const VkCommandBufferInheritanceInfo& inheritanceInfo,
		const uint32_t imageIndex, const VulkanContextInfo& contextInfo, const VulkanRenderPass& renderPass);
	bool endRecordingSecondary(const uint32_t imageIndex);

	void VulkanGraphicsPipeline::getViewportAndScissor(VkViewport& outViewport, VkRect2D& outScissor, 
		const VulkanContextInfo& contextInfo, const uint32_t camIndex, const bool vrmode);

	void VulkanGraphicsPipeline::recordCommandBuffer(const uint32_t imageIndex, const VulkanContextInfo& contextInfo,
		const VulkanRenderPass& renderPass, const Model& model, const Mesh& mesh, const bool vrmode);
	void beginRecording(const uint32_t imageIndex, const VulkanContextInfo& contextInfo, const VulkanRenderPass& renderPass);
	bool endRecording(const uint32_t imageIndex);
	void VulkanGraphicsPipeline::recordCommandBufferPrimary(const VkCommandBuffer& singleCmdBuffer,
		const uint32_t imageIndex, const VulkanContextInfo& contextInfo, const Model& model, const Mesh& mesh, const bool vrmode);
	void createSemaphores(const VulkanContextInfo& contextInfo);

	//cleanup
	void freeCommandBuffers(const VulkanContextInfo& contextInfo);
	void destroyPipeline(const VulkanContextInfo& contextInfo);
	void destroyPipelineLayout(const VulkanContextInfo& contextInfo);
	void destroyPipelineSemaphores(const VulkanContextInfo& contextInfo);
	void destroyVulkanPipeline(const VulkanContextInfo& contextInfo);
};

