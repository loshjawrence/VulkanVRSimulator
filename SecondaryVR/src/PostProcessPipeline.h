#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "VulkanContextInfo.h"
#include "VulkanDescriptor.h"
#include "VulkanRenderPass.h"
#include "VulkanImage.h"
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

enum class PipelineType {
	PP = 0, TIMEWARP = 1
};
struct PostProcessPushConstant {
	uint32_t toggleFlags;
	uint32_t virtualWidth;
	uint32_t virtualHeight;
	static const VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
};

struct TimeWarpPushConstant {
	glm::mat4 timeWarpInvVP;
	uint32_t toggleFlags;
	uint32_t virtualWidth;
	uint32_t virtualHeight;
	static const VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
};

class PostProcessPipeline {
public:
	std::vector<std::string> shaderpaths;
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkCommandPool> commandPools;

	//NEW
	std::vector<VulkanImage> outputImages;//give to this stage's framebuffers and next stage's inputDescriptors
	std::vector<VkFramebuffer> framebuffers;
	std::vector<VulkanDescriptor> inputDescriptors;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	//recording state
	bool recording = false;

	//is Last post process
	bool isPresent;
	PipelineType pipelinetype;//0 is normal, 1 is timewarp

public:
	PostProcessPipeline();
	PostProcessPipeline(const std::vector<std::string>& shaderspaths, const VulkanRenderPass& renderPass,
		const VulkanContextInfo& contextInfo, const VkDescriptorSetLayout* setLayouts, const bool isPresent,
	const PipelineType type);

	~PostProcessPipeline();

	//NEW
	void createOutputImages(const VulkanContextInfo& contextInfo);
	void addCommandPools(const VulkanContextInfo& contextInfo, const uint32_t num);
	void createFramebuffers(const VulkanContextInfo& contextInfo, const VulkanRenderPass& renderPass);
	void createInputDescriptors(const VulkanContextInfo& contextInfo, const std::vector<VulkanImage>& vulkanImages);
	//void createInputDescriptorsTimeWarp(const VulkanContextInfo& contextInfo, const std::vector<VulkanImage>& vulkanImages, const VulkanImage& depthImage);
	void createInputDescriptorsTimeWarp(const VulkanContextInfo& contextInfo,
		const std::vector<VulkanImage>& vulkanImages, const VulkanImage& depthImage, const VkBuffer& uniformBuffer,
		const int sizeofUBOstruct);
	//void createStaticCommandBuffers(const VulkanContextInfo& contextInfo,
	//	const VulkanRenderPass& renderPass, const Mesh& mesh, const bool vrmode);
	void createStaticCommandBuffers(const VulkanContextInfo& contextInfo,
		const VulkanRenderPass& renderPass, const std::vector<Mesh>& meshes);//static since no dynamic input, mesh is just quad or triangle
	void createStaticCommandBuffersTimeWarp(const VulkanContextInfo& contextInfo,
		const VulkanRenderPass& renderPass, const std::vector<Mesh>& meshes);


	void allocateCommandBuffers(const VulkanContextInfo& contextInfo);

	void createPipeline(const VulkanRenderPass& renderPass, const VulkanContextInfo& contextInfo, 
		const VkDescriptorSetLayout* setLayouts);

	VkShaderModule createShaderModule(const std::vector<char>& code, const VulkanContextInfo& contextInfo) const;

	////SECONDARY RECORDING////
	void recordCommandBufferSecondary(const VkCommandBufferInheritanceInfo& inheritanceInfo,
		const uint32_t imageIndex, const VulkanContextInfo& contextInfo, const Model& model, 
		const Mesh& mesh, const bool vrmode);
	void beginRecordingSecondary(const VkCommandBufferInheritanceInfo& inheritanceInfo,
		const uint32_t imageIndex, const VulkanContextInfo& contextInfo);
	bool endRecordingSecondary(const uint32_t imageIndex);

	void getViewportAndScissor(VkViewport& outViewport, VkRect2D& outScissor, 
		const VulkanContextInfo& contextInfo, const uint32_t camIndex, const bool vrmode);

	void recordCommandBufferPrimary(const VkCommandBuffer& singleCmdBuffer,
		const uint32_t imageIndex, const VulkanContextInfo& contextInfo, const Model& model, const Mesh& mesh, const bool vrmode);
	void createSemaphores(const VulkanContextInfo& contextInfo);

	//cleanup
	void freeCommandBuffers(const VulkanContextInfo& contextInfo);
	void destroyPipeline(const VulkanContextInfo& contextInfo);
	void destroyPipelineLayout(const VulkanContextInfo& contextInfo);
	void destroyPipelineSemaphores(const VulkanContextInfo& contextInfo);
	void destroyVulkanPipeline(const VulkanContextInfo& contextInfo);
};


