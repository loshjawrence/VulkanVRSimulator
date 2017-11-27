#pragma once
#include "VulkanGraphicsPipeline.h"

#include "Utils.h"
#include "Model.h";

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>


VulkanGraphicsPipeline::VulkanGraphicsPipeline() {

}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(const std::vector<std::string>& shaderpaths,
	const VulkanRenderPass& renderPass, const VulkanContextInfo& contextInfo, const VkDescriptorSetLayout* setLayouts) 
	: shaderpaths(shaderpaths)
{
	createGraphicsPipeline(renderPass, contextInfo, setLayouts);
	allocateCommandBuffers(contextInfo);
	createSemaphores(contextInfo);
}


VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
}


void VulkanGraphicsPipeline::allocateCommandBuffers(const VulkanContextInfo& contextInfo) {
	commandBuffers.resize(contextInfo.swapChainFramebuffers.size());
	//clean up before record? no need, state is not maintained
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = contextInfo.graphicsCommandPools[0];
	//allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandBufferCount = commandBuffers.size();

	if (vkAllocateCommandBuffers(contextInfo.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to alloc pipeline command buffers!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanGraphicsPipeline::createGraphicsPipeline(const VulkanRenderPass& renderPass,
	const VulkanContextInfo& contextInfo, const VkDescriptorSetLayout* setLayouts)
{
	auto vertShaderCode = readFile(shaderpaths[0]);
	auto fragShaderCode = readFile(shaderpaths[1]);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, contextInfo);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, contextInfo);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	//TODO: for dynamic amount of shaders, turn into vector pass .data to the pipeline info at the bottom
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)contextInfo.swapChainExtent.width;
	viewport.height = (float)contextInfo.swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = contextInfo.swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	rasterizer.lineWidth = 1.0f;
	//rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//////////////////////////////
	//// PUSH CONSTANT RANGES ////
	//////////////////////////////
	std::vector<VkPushConstantRange> pushContantRanges;
	VkPushConstantRange push1 = {};
	push1.offset = 0;
	push1.size = sizeof(ForwardPushConstant);
	push1.stageFlags = ForwardPushConstant::stages;
	pushContantRanges.push_back(push1);

	///////////////////////
	//// DYNAMIC STATE ////
	///////////////////////
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	std::vector<VkDynamicState> dynamicStates;
	dynamicStates.push_back( VK_DYNAMIC_STATE_VIEWPORT );
	dynamicStates.push_back( VK_DYNAMIC_STATE_SCISSOR );
	//dynamicStates.push_back( VK_DYNAMIC_STATE_STENCIL_REFERENCE );//also: compare_mask?
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pNext = nullptr;
	dynamicInfo.flags = 0;
	dynamicInfo.dynamicStateCount = dynamicStates.size();
	dynamicInfo.pDynamicStates = &dynamicStates[0];


	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = setLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = pushContantRanges.size();
	pipelineLayoutInfo.pPushConstantRanges = pushContantRanges.data();

	if (vkCreatePipelineLayout(contextInfo.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create pipeline layout!";
		throw std::runtime_error(ss.str());
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.pDynamicState = &dynamicInfo;

	if (vkCreateGraphicsPipelines(contextInfo.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create graphics pipeline!";
		throw std::runtime_error(ss.str());
	}

	vkDestroyShaderModule(contextInfo.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(contextInfo.device, vertShaderModule, nullptr);
}

VkShaderModule VulkanGraphicsPipeline::createShaderModule( const std::vector<char>& code, 
	const VulkanContextInfo& contextInfo) const 
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(contextInfo.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create shader module!";
		throw std::runtime_error(ss.str());
	}

	return shaderModule;
}

void VulkanGraphicsPipeline::recordCommandBufferSecondary(const VkCommandBufferInheritanceInfo& inheritanceInfo,
	const uint32_t imageIndex, const VulkanContextInfo& contextInfo, const Model& model, 
	const Mesh& mesh, const bool vrmode)
{

	if (!recording) {
		beginRecordingSecondary(inheritanceInfo, imageIndex);
	} 

	const VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
	const VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffers[imageIndex], mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mesh.descriptor.descriptorSet, 0, nullptr);

	const int camIndex = 0;
	const ForwardPushConstant pushconstant = { model.modelMatrix, camIndex << 1 | model.isDynamic };
	vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, ForwardPushConstant::stages, 0, sizeof(ForwardPushConstant), (const void*)&pushconstant);

	VkViewport viewport = {}; VkRect2D scissor = {};
	getViewportAndScissor(viewport, scissor, contextInfo, camIndex, vrmode);
	vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

	vkCmdDrawIndexed(commandBuffers[imageIndex], static_cast<uint32_t>(mesh.mIndices.size()), 1, 0, 0, 0);

	if (vrmode) {
		const uint32_t camIndex = 1;
		const ForwardPushConstant pushconstant = { model.modelMatrix, uint32_t( camIndex << 1 | model.isDynamic ) };
		vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, ForwardPushConstant::stages, 0, sizeof(ForwardPushConstant), (const void*)&pushconstant);

		getViewportAndScissor(viewport, scissor, contextInfo, camIndex, vrmode);
		vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
		vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

		vkCmdDrawIndexed(commandBuffers[imageIndex], static_cast<uint32_t>(mesh.mIndices.size()), 1, 0, 0, 0);
	}
}

void VulkanGraphicsPipeline::beginRecordingSecondary(const VkCommandBufferInheritanceInfo& inheritanceInfo,
	uint32_t imageIndex) 
{
	recording = true;
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = &inheritanceInfo;

	vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo);

	vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}


bool VulkanGraphicsPipeline::endRecordingSecondary(const uint32_t imageIndex) {
	if (recording) {
		recording = false;

		if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
			std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to record command buffer!";
			throw std::runtime_error(ss.str());
		}
		return true;
	}
	return false;
}

void VulkanGraphicsPipeline::recordCommandBufferPrimary(const VkCommandBuffer& primaryCmdBuffer, 
	const uint32_t imageIndex, const VulkanContextInfo& contextInfo,
	 const Model& model, const Mesh& mesh, const bool vrmode)
{
	vkCmdBindPipeline(primaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	const VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
	const VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(primaryCmdBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(primaryCmdBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(primaryCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mesh.descriptor.descriptorSet, 0, nullptr);

	const uint32_t camIndex = 0;
	const ForwardPushConstant pushconstant = { model.modelMatrix, uint32_t( camIndex << 1 | model.isDynamic )};
	vkCmdPushConstants(primaryCmdBuffer, pipelineLayout, ForwardPushConstant::stages, 0, sizeof(ForwardPushConstant), (const void*)&pushconstant);

	VkViewport viewport = {}; VkRect2D scissor = {};
	getViewportAndScissor(viewport, scissor, contextInfo, camIndex, vrmode);
	vkCmdSetViewport(primaryCmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(primaryCmdBuffer, 0, 1, &scissor);

	vkCmdDrawIndexed(primaryCmdBuffer, static_cast<uint32_t>(mesh.mIndices.size()), 1, 0, 0, 0);

	if (vrmode) {
		const uint32_t camIndex = 1;
		const ForwardPushConstant pushconstant = { model.modelMatrix, uint32_t( camIndex << 1 | model.isDynamic ) };
		vkCmdPushConstants(primaryCmdBuffer, pipelineLayout, ForwardPushConstant::stages, 0, sizeof(ForwardPushConstant), (const void*)&pushconstant);

		getViewportAndScissor(viewport, scissor, contextInfo, camIndex, vrmode);
		vkCmdSetViewport(primaryCmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(primaryCmdBuffer, 0, 1, &scissor);

		vkCmdDrawIndexed(primaryCmdBuffer, static_cast<uint32_t>(mesh.mIndices.size()), 1, 0, 0, 0);
	}
}

void VulkanGraphicsPipeline::getViewportAndScissor(VkViewport& outViewport, VkRect2D& outScissor, 
	const VulkanContextInfo& contextInfo, const uint32_t camIndex, const bool vrmode) {
	outViewport.minDepth = 0.0f;
	outViewport.maxDepth = 1.0f;

	if (vrmode) {
		outViewport.width = (float)contextInfo.swapChainExtent.width * 0.5f;
		outViewport.height = (float)contextInfo.swapChainExtent.height;
		if (camIndex == 0) {
			outViewport.x = 0.0f;
			outViewport.y = 0.0f;
			outScissor.offset = { (int32_t )outViewport.x,		(int32_t )outViewport.y };
			outScissor.extent = { (uint32_t)outViewport.width,	(uint32_t)outViewport.height };
		} else {
			outViewport.x = outViewport.width;
			outViewport.y = 0.0f;
			outScissor.offset = { (int32_t )outViewport.x,		(int32_t )outViewport.y };
			outScissor.extent = { (uint32_t)outViewport.width,	(uint32_t)outViewport.height };
		}
	} else {
		outViewport.x = 0.0f;
		outViewport.y = 0.0f;
		outViewport.width = (float)contextInfo.swapChainExtent.width;
		outViewport.height = (float)contextInfo.swapChainExtent.height;

		outScissor.offset = { 0, 0 };
		outScissor.extent = contextInfo.swapChainExtent;
	}
}



void VulkanGraphicsPipeline::createSemaphores(const VulkanContextInfo& contextInfo) {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(contextInfo.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(contextInfo.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) 
	{
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create semaphores!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanGraphicsPipeline::destroyVulkanPipeline(const VulkanContextInfo& contextInfo) {
	freeCommandBuffers(contextInfo);
	destroyPipeline(contextInfo);
	destroyPipelineLayout(contextInfo);
}

void VulkanGraphicsPipeline::freeCommandBuffers(const VulkanContextInfo& contextInfo) {
	vkFreeCommandBuffers(contextInfo.device, contextInfo.graphicsCommandPools[0], static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
}

void VulkanGraphicsPipeline::destroyPipeline(const VulkanContextInfo& contextInfo) {
	vkDestroyPipeline(contextInfo.device, graphicsPipeline, nullptr);
}

void VulkanGraphicsPipeline::destroyPipelineLayout(const VulkanContextInfo& contextInfo) {
	vkDestroyPipelineLayout(contextInfo.device, pipelineLayout, nullptr);
}

void VulkanGraphicsPipeline::destroyPipelineSemaphores(const VulkanContextInfo& contextInfo) {
	vkDestroySemaphore(contextInfo.device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(contextInfo.device, imageAvailableSemaphore, nullptr);
}

