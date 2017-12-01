#pragma once
#include "VulkanApplication.h"
//#define _CRT_SECURE_NO_WARNINGS 1 
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN


#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "GlobalSettings.h"
#include "Utils.h"

#include <fstream>
#include <chrono>
#include <cstring>
#include <tuple>

std::string convertIntToString(int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

std::string convertFloatToString(double number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

void VulkanApplication::loadModels() {
	const int num = 1;
	const int numMeshesPerStride = 1;
	std::vector< std::tuple<std::string, int, glm::mat4> > defaultScene(num);
	for (int i = 0; i < num/numMeshesPerStride; i += numMeshesPerStride) {
		const float x = static_cast<float>(rng.nextUInt(1));
		const float y = static_cast<float>(rng.nextUInt(1));
		const float z = static_cast<float>(rng.nextUInt(1));
		defaultScene[i] = { std::string("res/objects/rock/rock.obj"), 1,
			glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.0f)),glm::vec3(x, y, z)) };
		//defaultScene[i+1] = { std::string("res/objects/cube.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.0f)),glm::vec3(y, z, x)) };
		//defaultScene[i] = { std::string("res/objects/buddha.obj"), 1,//Largest that works
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(5.0f)),glm::vec3(y, z, x)) };
		//defaultScene[i+1] = { std::string("res/objects/breakfast_room/breakfast_room.obj"), 0,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.5f)),glm::vec3(x, y, z)) };
		//defaultScene[i] = { std::string("res/objects/cerberus_maximov/source/Cerberus_LP.FBX.fbx"), 1,
			//glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x, y, z)) };
		//defaultScene[i] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x, y, z)) };
		//defaultScene[i+2] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x+1, y, z)) };
		//defaultScene[i+3] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x+2, y, z)) };
		//defaultScene[i+1] = { std::string("res/objects/cryteksponza/sponza.obj"), 0,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.005f)),glm::vec3(x, y, z)) };
		//defaultScene[i] = { std::string("res/objects/dabrovicsponza/sponza.obj"), 0,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.0f)),glm::vec3(x, y, z)) };
		//defaultScene[i] = { std::string("res/objects/sibenikcathedral/sibenik.obj"), 0,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.0f)),glm::vec3(x, y, z)) };
	}

	for (auto& modelinfo : defaultScene) {//defaultScene in GlobalSettings.h
		models.push_back(Model(std::get<0>(modelinfo), std::get<1>(modelinfo), std::get<2>(modelinfo),
			contextInfo, uniformBuffer, uniformBufferMemory, sizeof(UniformBufferObject)));
	}
}

void VulkanApplication::initVulkan() {
	
	//make a GLFW window
	initWindow();
	//createRadialStencilMask();//no need to generate everytime

	//context info holds vulkan things like instance, phys and logical device, swap chain info, depthImage, command pools and queues
	contextInfo = VulkanContextInfo(window,std::string("radialStencilMask.bmp"));
	VulkanApplication::setupDebugCallback();

	
	//describes input and output attachments and how subpasses relate to one another
	allRenderPasses = VulkanRenderPass(contextInfo);

	//really just for initializing static VulkanDescriptor::"pipeline"LayoutTypes vectors
	VulkanDescriptor justInit = VulkanDescriptor(contextInfo);

	//contextInfo.createSwapChainFramebuffers(allRenderPasses.renderPass);
	contextInfo.createSwapChainFramebuffers(allRenderPasses.renderPassPostProcessPresent);

	//defualtMeshes
	ndcTriangle = Mesh(contextInfo, MESHTYPE::NDCTRIANGLE);//ndc triangle for post processing
	ndcBarrelMesh[0] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH, 0);
	ndcBarrelMesh[1] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH, 1);
	ndcBarrelMesh_PreCalc[0] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH_PRECALC, 0);
	ndcBarrelMesh_PreCalc[1] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH_PRECALC, 1);

	//setup all pipelines
	createPipelines();
	VulkanBuffer::createUniformBuffer(contextInfo, sizeof(UniformBufferObject), uniformBuffer, uniformBufferMemory);

	loadModels();

	createSemaphores();
	allocateGlobalCommandBuffers();
}


void VulkanApplication::drawFrame() {
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(contextInfo.device, contextInfo.swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to acquire swap chain image!";
		throw std::runtime_error(ss.str());
	}

	std::vector<VkSemaphore> forwardWaitSemaphores = { imageAvailableSemaphore };
	std::vector<VkPipelineStageFlags> forwardWaitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	updateUniformBuffer();
	//////////////////
	//// FORWARD /////
	//////////////////
	//PRIMARY FILLED WITH HOMOGENOUS SECONDARY
	//VkCommandBufferInheritanceInfo inheritanceInfo = {};
	//beginRecordingPrimary(inheritanceInfo, imageIndex);
	//for (Model& model : models) {
	//	//record command buffers for visible objects
	//	for (Mesh& mesh : model.mMeshes) {
	//	    //TODO: the pipeline selection is wrong
	//		const uint32_t index = getForwardPipelineIndexFromTextureMapFlags(mesh.descriptor.textureMapFlags);
	//		forwardPipelines[index].recordCommandBufferSecondary(
	//			inheritanceInfo, imageIndex, contextInfo, model, mesh, contextInfo.camera.vrmode);
	//	}
	//}

	////gather vector of pipeline homogenous secondary command buffers
	//std::vector<VkCommandBuffer> forwardCommandBuffers;
	//for (int i = 0; i < forwardPipelines.size(); ++i) {
	//	if (forwardPipelines[i].endRecordingSecondary(imageIndex)) {
	//		forwardCommandBuffers.push_back(forwardPipelines[i].commandBuffers[imageIndex]);
	//	}
	//}
	////record into primary buffer
	//vkCmdExecuteCommands(primaryForwardCommandBuffers[imageIndex],
	//	static_cast<uint32_t>(forwardCommandBuffers.size()), forwardCommandBuffers.data());
	//endRecordingPrimary(imageIndex);




	//////Primary bufer recorded directly with unsorted pipelines
	beginRecordingPrimary(imageIndex);
	for (Model& model : models) {
		//TODO: record only visible meshes
		for (Mesh& mesh : model.mMeshes) {
	      //TODO: the pipeline selection is wrong
			const uint32_t index = getForwardPipelineIndexFromTextureMapFlags(mesh.descriptor.textureMapFlags);
			forwardPipelines[index].recordCommandBufferPrimary(
				primaryForwardCommandBuffers[imageIndex], imageIndex, contextInfo, model, mesh, contextInfo.camera.vrmode);
		}
	}
	endRecordingPrimary(imageIndex);


	VkSubmitInfo forwardSubmitInfo = {};
	forwardSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	forwardSubmitInfo.waitSemaphoreCount = forwardWaitSemaphores.size();
	forwardSubmitInfo.pWaitSemaphores = &forwardWaitSemaphores[0];
	forwardSubmitInfo.pWaitDstStageMask = &forwardWaitStages[0];

	forwardSubmitInfo.commandBufferCount = 1;
	forwardSubmitInfo.pCommandBuffers = &primaryForwardCommandBuffers[imageIndex];

	std::vector<VkSemaphore> forwardSignalSemaphores = { forwardRenderFinishedSemaphore };
	forwardSubmitInfo.signalSemaphoreCount = forwardSignalSemaphores.size();
	forwardSubmitInfo.pSignalSemaphores = &forwardSignalSemaphores[0];

	if (vkQueueSubmit(contextInfo.graphicsQueue, 1, &forwardSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to submit draw command buffer!";
		throw std::runtime_error(ss.str());
	}

	///////////////////////////////
	//////// POST PROCESS /////////
	///////////////////////////////
	VkSubmitInfo postProcessSubmitInfo = {};
	postProcessSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	std::vector<VkSemaphore> postProcessWaitSemaphores = { forwardRenderFinishedSemaphore };
	std::vector<VkPipelineStageFlags> postProcessWaitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	postProcessSubmitInfo.waitSemaphoreCount = postProcessWaitSemaphores.size();
	postProcessSubmitInfo.pWaitDstStageMask = &postProcessWaitStages[0];
	postProcessSubmitInfo.commandBufferCount = 1;

	for (auto& pipeline : postProcessPipelines) {
		postProcessSubmitInfo.pWaitSemaphores = &postProcessWaitSemaphores[0];
		postProcessSubmitInfo.pCommandBuffers = &pipeline.commandBuffers[imageIndex];
		std::vector<VkSemaphore> postProcessSignalSemaphores = { pipeline.renderFinishedSemaphore };
		postProcessSubmitInfo.signalSemaphoreCount = postProcessSignalSemaphores.size();
		postProcessSubmitInfo.pSignalSemaphores = &postProcessSignalSemaphores[0];

		if (vkQueueSubmit(contextInfo.graphicsQueue, 1, &postProcessSubmitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to submit draw command buffer!";
			throw std::runtime_error(ss.str());
		}
		postProcessWaitSemaphores = { pipeline.renderFinishedSemaphore };
	}
	




	///////////////////////
	/////// PRESENT////////
	///////////////////////
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	std::vector<VkSemaphore> presentSignalSemaphores = { postProcessPipelines.back().renderFinishedSemaphore };
	presentInfo.waitSemaphoreCount = presentSignalSemaphores.size();
	presentInfo.pWaitSemaphores = &presentSignalSemaphores[0];
	//presentInfo.waitSemaphoreCount = forwardSignalSemaphores.size();
	//presentInfo.pWaitSemaphores = &forwardSignalSemaphores[0];

	VkSwapchainKHR swapChains[] = { contextInfo.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vkQueueWaitIdle(contextInfo.presentQueue);
	result = vkQueuePresentKHR(contextInfo.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to present swap chain image!";
		throw std::runtime_error(ss.str());
	}
}


void VulkanApplication::beginRecordingPrimary(VkCommandBufferInheritanceInfo& inheritanceInfo, const uint32_t imageIndex) {

	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = NULL;
	//inheritanceInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	inheritanceInfo.framebuffer = forwardPipelinesFramebuffers[imageIndex];
	inheritanceInfo.renderPass = allRenderPasses.renderPass;
	inheritanceInfo.occlusionQueryEnable = VK_FALSE;
	inheritanceInfo.pipelineStatistics = 0;


	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(primaryForwardCommandBuffers[imageIndex], &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = allRenderPasses.renderPass;
	//renderPassInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	renderPassInfo.framebuffer = forwardPipelinesFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
//	renderPassInfo.renderArea.extent = contextInfo.swapChainExtent;
	VkExtent2D renderTargetExent;
	float vrScaleXback = contextInfo.camera.vrmode ? 2.f : 1.f;
	renderTargetExent.width = vrScaleXback * static_cast<uint32_t>(contextInfo.camera.width);
	renderTargetExent.height = static_cast<uint32_t>(contextInfo.camera.height);
	renderPassInfo.renderArea.extent.height = renderTargetExent.height;
	renderPassInfo.renderArea.extent.width = renderTargetExent.width;
	//renderPassInfo.renderArea.extent.width = static_cast<uint32_t>(contextInfo.camera.width);
	//renderPassInfo.renderArea.extent.height = static_cast<uint32_t>(contextInfo.camera.height);

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 1 };
	//clearValues[1].depthStencil = { 1.0f };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(primaryForwardCommandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void VulkanApplication::beginRecordingPrimary(const uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(primaryForwardCommandBuffers[imageIndex], &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = allRenderPasses.renderPass;
	//renderPassInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	renderPassInfo.framebuffer = forwardPipelinesFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	//renderPassInfo.renderArea.extent = contextInfo.swapChainExtent;
	VkExtent2D renderTargetExent;
	float vrScaleXback = contextInfo.camera.vrmode ? 2.f : 1.f;
	renderTargetExent.width = vrScaleXback * static_cast<uint32_t>(contextInfo.camera.width);
	renderTargetExent.height = static_cast<uint32_t>(contextInfo.camera.height);
	renderPassInfo.renderArea.extent.height = renderTargetExent.height;
	renderPassInfo.renderArea.extent.width = renderTargetExent.width;
	//renderPassInfo.renderArea.extent.height = static_cast<uint32_t>(contextInfo.camera.height);
	//renderPassInfo.renderArea.extent.width = static_cast<uint32_t>(contextInfo.camera.width);

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	//clearValues[1].depthStencil = { 1.f, 1 };
	clearValues[1].depthStencil = { 1.f };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(primaryForwardCommandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}


void VulkanApplication::endRecordingPrimary(const uint32_t imageIndex) {
	vkCmdEndRenderPass(primaryForwardCommandBuffers[imageIndex]);
	if (vkEndCommandBuffer(primaryForwardCommandBuffers[imageIndex]) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to record command buffer!";
		throw std::runtime_error(ss.str());
	}
}



void VulkanApplication::createSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(contextInfo.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(contextInfo.device, &semaphoreInfo, nullptr, &forwardRenderFinishedSemaphore) != VK_SUCCESS)
	{
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create semaphores!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanApplication::updateUniformBuffer() {
	UniformBufferObject ubo = {};
	//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view[0] = contextInfo.camera.view[0];
	ubo.view[1] = contextInfo.camera.view[1];
	ubo.proj = contextInfo.camera.proj;
	ubo.proj[1][1] *= -1;//need for correct z-buffer order
	ubo.time = time;

	void* data;
	vkMapMemory(contextInfo.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(contextInfo.device, uniformBufferMemory);
}

void VulkanApplication::allocateGlobalCommandBuffers() {
	primaryForwardCommandBuffers.resize(contextInfo.swapChainFramebuffers.size());
	addGraphicsCommandPool(primaryForwardCommandBuffers.size());
	//clean up before record? no need, state is not maintained
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = contextInfo.graphicsCommandPools[0];
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = primaryForwardCommandBuffers.size();

	if (vkAllocateCommandBuffers(contextInfo.device, &allocInfo, primaryForwardCommandBuffers.data()) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to alloc pipeline command buffers!";
		throw std::runtime_error(ss.str());
	}
}

void VulkanApplication::addGraphicsCommandPool(const int num) {
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = contextInfo.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	for (int i = 0; i < num; ++i) {
		VkCommandPool commandPool;
		graphicsCommandPools.push_back(commandPool);

		if (vkCreateCommandPool(contextInfo.device, &poolInfo, nullptr, &graphicsCommandPools.back()) != VK_SUCCESS) {
			std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create graphics command pool!";
			throw std::runtime_error(ss.str());
		}
	}
}


void VulkanApplication::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		processInputAndUpdateFPS();
		drawFrame();
	}
	vkDeviceWaitIdle(contextInfo.device);
}

void VulkanApplication::updateFPS() {
	currenttime = glfwGetTime();
	fpstracker++;
	const double elapsed = currenttime - oldtime;
	if (elapsed >= 1) {
		fps = (int)(fpstracker / elapsed);
		fpstracker = 0;
		oldtime = currenttime;
		std::string title = "render | " + convertIntToString(fps) + " FPS " + convertFloatToString(1000.f / (double)fps) + " ms";
		glfwSetWindowTitle(window, title.c_str());
	}
}

void VulkanApplication::processInputAndUpdateFPS() {
	// ask glfw for keys pressed
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;


		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		updateFPS();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			contextInfo.camera.processKeyboardAndUpdateView(MovementDirection::FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			contextInfo.camera.processKeyboardAndUpdateView(MovementDirection::BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			contextInfo.camera.processKeyboardAndUpdateView(MovementDirection::LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			contextInfo.camera.processKeyboardAndUpdateView(MovementDirection::RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			contextInfo.camera.processKeyboardAndUpdateView(MovementDirection::UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			contextInfo.camera.processKeyboardAndUpdateView(MovementDirection::DOWN, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
			contextInfo.camera.updateVrModeAndCameras();
			recreateSwapChain();
		}
}

void VulkanApplication::cleanup() {
	cleanupSwapChain();

	//TODO: add to Mesh cleanup texture image
	for (auto& model : models) {
		model.destroyVulkanHandles(contextInfo);
	}

	//clean up pipeline semaphores
	destroyPipelinesSemaphores();

	//clean up command pools
	contextInfo.destroyCommandPools();

	//clean up logical device, debug callback surface, instance
	contextInfo.destroyDevice();
	DestroyDebugReportCallbackEXT(callback, nullptr);
	contextInfo.destroySurface();
	contextInfo.destroyInstance();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void VulkanApplication::destroyPipelinesSemaphores() {
	for (auto& pipeline : forwardPipelines) {
		pipeline.destroyPipelineSemaphores(contextInfo);
	}
	for (auto& pipeline : postProcessPipelines) {
		pipeline.destroyPipelineSemaphores(contextInfo);
	}
}

void VulkanApplication::createPipelines() {
	//forwardPipelines
	forwardPipelines.resize(allShaders_ForwardPipeline.size());
	textureMapFlagsToForwardPipelineIndex.resize(allShaders_ForwardPipeline.size());

	for (uint32_t i = 0; i < allShaders_ForwardPipeline.size(); ++i) {
		int numImageSamplers = 0;
		for (int k = 1; k < VulkanDescriptor::MAX_IMAGESAMPLERS + 1; ++k) //the first bit is for HAS_NONE so we need to ignore that one
			numImageSamplers = (allShaders_ForwardPipeline[i].second & 1 << k) ? numImageSamplers + 1 : numImageSamplers;

		textureMapFlagsToForwardPipelineIndex[i] = allShaders_ForwardPipeline[i].second;//create the mapping based on shaders we have
		forwardPipelines[i] = VulkanGraphicsPipeline(allShaders_ForwardPipeline[i].first,
			allRenderPasses, contextInfo, &(VulkanDescriptor::layoutTypes[numImageSamplers]));
	}

	//post process pipelines
	postProcessPipelines.resize(allShaders_PostProcessPipeline.size());
	for (uint32_t i = 0; i < allShaders_PostProcessPipeline.size(); ++i) {
		const uint32_t numImageSamplers = allShaders_PostProcessPipeline[i].second;

		//need an mapping from numimagesamplers to layouttypes index
		//TODO: If last post process then outputImage format should be swapchain format, otherwise 16F
		postProcessPipelines[i] = PostProcessPipeline(allShaders_PostProcessPipeline[i].first,
			allRenderPasses, contextInfo, &(VulkanDescriptor::postProcessLayoutTypes[numImageSamplers-1]));
	}

	//each pp needs inputdescriptor set ofprevious stage
	initForwardPipelinesVulkanImagesAndFramebuffers();
	postProcessPipelines[0].createInputDescriptors(contextInfo, forwardPipelinesVulkanImages);
	for (int i = 1; i < postProcessPipelines.size(); ++i) {
		postProcessPipelines[i].createInputDescriptors(contextInfo, postProcessPipelines[i-1].outputImages);
	}

	//create the static command buffers(no dynamic input for post processing)
	std::vector<Mesh> ppMeshes; 
	if (contextInfo.camera.vrmode) {
		//ppMeshes.push_back(ndcBarrelMesh_PreCalc[0]);
		//ppMeshes.push_back(ndcBarrelMesh_PreCalc[1]);
		ppMeshes.push_back(ndcBarrelMesh[0]);
		ppMeshes.push_back(ndcBarrelMesh[1]);
		//ppMeshes.push_back(ndcTriangle);
		//ppMeshes.push_back(ndcTriangle);
	} else {
		ppMeshes.push_back(ndcTriangle);
	}

	for (auto& pipeline : postProcessPipelines) {
		//pipeline.createStaticCommandBuffers(contextInfo, allRenderPasses, ndcBarrelMesh, contextInfo.camera.vrmode);
		pipeline.createStaticCommandBuffers(contextInfo, allRenderPasses, ppMeshes, contextInfo.camera.vrmode);
	}


}

void VulkanApplication::initForwardPipelinesVulkanImagesAndFramebuffers() {
	forwardPipelinesVulkanImages.resize(contextInfo.swapChainImages.size());
	forwardPipelinesFramebuffers.resize(contextInfo.swapChainImages.size());
	VkExtent2D renderTargetExent;
	float vrScaleXback = contextInfo.camera.vrmode ? 2.f : 1.f;
	renderTargetExent.width = vrScaleXback * static_cast<uint32_t>(contextInfo.camera.width);
	renderTargetExent.height = static_cast<uint32_t>(contextInfo.camera.height);
	for (int i = 0; i < contextInfo.swapChainImages.size(); ++i) {
		//forwardPipelinesVulkanImages[i].image = contextInfo.swapChainImages[i];
		//forwardPipelinesVulkanImages[i].imageView = contextInfo.swapChainImageViews[i];
		//forwardPipelinesVulkanImages[i].extent = contextInfo.swapChainExtent;
		//forwardPipelinesVulkanImages[i].format = contextInfo.swapChainImageFormat;

		//TODO: if flag is present then use swapchain format otherwise 16F
		//FOR LATER://these will need to be rebuilt in recreateswapchain?
		//forwardPipelinesVulkanImages[i] = VulkanImage(IMAGETYPE::COLOR_ATTACHMENT, contextInfo.swapChainExtent, VK_FORMAT_R16G16B16A16_SFLOAT, contextInfo);
		forwardPipelinesVulkanImages[i] = VulkanImage(IMAGETYPE::COLOR_ATTACHMENT, renderTargetExent, VK_FORMAT_R16G16B16A16_SFLOAT, contextInfo);

	}

	//for (int i = 0; i < contextInfo.swapChainImages.size(); ++i) {
	//	forwardPipelinesFramebuffers[i] = contextInfo.swapChainFramebuffers[i];
	//}

	////FOR LATER://these will need to be rebuild in recreateswapchain?
	for (int i = 0; i < contextInfo.swapChainImages.size(); ++i) {
		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = NULL;

		//TODO: if last make present otherwise normal post process
		std::vector<VkImageView> attachments = { forwardPipelinesVulkanImages[i].imageView, contextInfo.depthImage.imageView };
		framebufferCreateInfo.renderPass = allRenderPasses.renderPass;
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.attachmentCount = attachments.size();
		//framebufferCreateInfo.width = contextInfo.swapChainExtent.width;
		//framebufferCreateInfo.height = contextInfo.swapChainExtent.height;
		//framebufferCreateInfo.width = static_cast<uint32_t>(contextInfo.camera.width);
		//framebufferCreateInfo.height = static_cast<uint32_t>(contextInfo.camera.height);
		framebufferCreateInfo.width = renderTargetExent.width;
		framebufferCreateInfo.height = renderTargetExent.height;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(contextInfo.device, &framebufferCreateInfo, nullptr, &forwardPipelinesFramebuffers[i]) != VK_SUCCESS) {
			std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create framebuffer!";
			throw std::runtime_error(ss.str());
		}
	}
}
uint32_t VulkanApplication::getForwardPipelineIndexFromTextureMapFlags(const uint32_t textureMapFlags) {
	for (uint32_t i = 0; i < textureMapFlagsToForwardPipelineIndex.size(); ++i) {
		if ((textureMapFlags & textureMapFlagsToForwardPipelineIndex[i]) == textureMapFlags) {
			return i;
		}
	}
}

void VulkanApplication::cleanupSwapChain() {
	contextInfo.depthImage.destroyVulkanImage(contextInfo);

	destroyPipelines();
	destroyOffScreenRenderTargets();
	freeGlobalCommandBuffers();

	allRenderPasses.destroyRenderPasses(contextInfo);//don't think this is needed for resize but needed for final cleanup

	contextInfo.destroyVulkanSwapChain();
}

void VulkanApplication::destroyOffScreenRenderTargets() {
	for (auto& image : forwardPipelinesVulkanImages) {
		image.destroyVulkanImage(contextInfo);
	}

	//dont need to do the last one since it refers to the swap chain
	for (int i = 0; i < postProcessPipelines.size() - 1; ++i) {
		for (auto& image : postProcessPipelines[i].outputImages) {
			image.destroyVulkanImage(contextInfo);
		}
	}
}

void VulkanApplication::freeGlobalCommandBuffers() {
	for(int i = 0; i < primaryForwardCommandBuffers.size(); ++i) {
		vkFreeCommandBuffers(contextInfo.device, contextInfo.graphicsCommandPools[0], 1, &primaryForwardCommandBuffers[i]);
	}
}

void VulkanApplication::destroyPipelines() {
	for (auto& pipeline : forwardPipelines) {
		pipeline.destroyVulkanPipeline(contextInfo);
	}
	for (auto& pipeline : postProcessPipelines) {
		pipeline.destroyVulkanPipeline(contextInfo);
	}
}


void VulkanApplication::recreateSwapChain() {
	vkDeviceWaitIdle(contextInfo.device);

	cleanupSwapChain();

	contextInfo.createSwapChain(window);
	contextInfo.createSwapChainImageViews();
	//update camera
	contextInfo.camera.updateDimensions(contextInfo.swapChainExtent);
	contextInfo.createDepthImage();

	allRenderPasses.createRenderPasses(contextInfo);


	contextInfo.createSwapChainFramebuffers(allRenderPasses.renderPassPostProcessPresent);
	allocateGlobalCommandBuffers();

	createPipelines();
}


void VulkanApplication::setupDebugCallback() {
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = VulkanApplication::debugCallback;

	if (CreateDebugReportCallbackEXT(&createInfo, nullptr, &callback) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to setup debug callback!";
		throw std::runtime_error(ss.str());
	}
}

VulkanApplication::VulkanApplication() {
}


VulkanApplication::~VulkanApplication() {
}

void VulkanApplication::run() {
	initVulkan();
	mainLoop();
	cleanup();
}

void VulkanApplication::initWindow() {
	if (!glfwInit()) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": glfwInit() failed!";
		throw std::runtime_error(ss.str());
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(startingWidth , startingHeight, "VulkanVR", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create glfw window!";
		throw std::runtime_error(ss.str());
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//tell glfw to capture mouse, for FPS camera
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//set callbacks and window user pointer
	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, VulkanApplication::onWindowResized);
	glfwSetCursorPosCallback(window, VulkanApplication::GLFW_MousePosCallback);
	glfwSetMouseButtonCallback(window, VulkanApplication::GLFW_MouseButtonCallback);
	glfwSetKeyCallback(window, VulkanApplication::GLFW_KeyCallback);
	glfwSetScrollCallback(window, VulkanApplication::GLFW_ScrollCallback);
}
VkResult VulkanApplication::CreateDebugReportCallbackEXT(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(contextInfo.instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(contextInfo.instance, pCreateInfo, pAllocator, &callback);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanApplication::DestroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(contextInfo.instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(contextInfo.instance, callback, pAllocator);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
	std::cerr << "validation layer: " << msg << std::endl;
	return VK_FALSE;
}

//GLFW callbacks, queried by glfwPollEvents();
// http://www.glfw.org/docs/3.0/group__input.html
void VulkanApplication::onWindowResized(GLFWwindow* window, int width, int height) {
	if (width == 0 || height == 0) return;

	VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
	app->recreateSwapChain();
}

void VulkanApplication::GLFW_MousePosCallback(GLFWwindow * window, double xpos, double ypos) {
	VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));

	if (app->firstmouse) {
		app->lastX = xpos;
		app->lastY = ypos;
		app->firstmouse = false;
	}

	float xoffset = xpos - app->lastX;
	float yoffset = app->lastY - ypos; // reversed since y-coordinates go from bottom to top for OpenGL

	app->lastX = xpos;
	app->lastY = ypos;

	app->contextInfo.camera.processMouseAndUpdateView(xoffset, yoffset);
}

void VulkanApplication::GLFW_MouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
}

void VulkanApplication::GLFW_KeyCallback(GLFWwindow * window, int key, int scanmode, int action, int mods) {
}

void VulkanApplication::GLFW_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
	app->contextInfo.camera.processScrollAndUpdateView(yoffset);
}

void VulkanApplication::createRadialStencilMask() {
	const float vrScaleXback = contextInfo.camera.vrmode ? 2.f : 1.f;
	const float width = vrScaleXback * contextInfo.camera.width;
	const float height = contextInfo.camera.height;
	const float invWidth = 1.f / width;
	const float invHeight = 1.f / height;
	const float vrMode = 1.f;
	const float middleRegionRadius = 0.52;//roughly 0.52
	const float NDCcenterOffset = 0.15;//0.15 ndc centeer UV center offset 0.0375
	const std::vector<glm::vec2> ndcCenter = { glm::vec2( NDCcenterOffset, 0.f), 
											   glm::vec2(-NDCcenterOffset, 0.f) };
	std::vector<std::vector<uint8_t>>radialDensityMask(3);
	radialDensityMask[0].resize(width*height);
	radialDensityMask[1].resize(width*height);
	radialDensityMask[2].resize(width*height);

	//go through all 2x2 set of pixels (center of group) and determine if that point samples outside of
	//the UV space for that eye, if so mark as 0 (z-near in vulkan). make a once that is blank for non vr mode?
	const uint32_t stencilMaskVal = 1;
	for (int camIndex = 0; camIndex <= 1; ++camIndex) {
		
		//x,y correspond to pixel number where 0,0 is upper left in vulkan
		//loop over entire render area for each eye, if outside of r=1.15 or render area set to stencilMask(need to XOR later to get middle cutout)
		for (int y = 1; y < height; y+=2) {
			for (int x = 1; x < width; x+=2) {
				//if (x == 881 && y == 601 && camIndex == 1) {
				//if (x == 1367 && y == 1 && camIndex == 1) {
				//	int adinvow = 1;
				//}

				//convert to uv
				glm::vec2 uv(x*invWidth, y*invHeight);

				//convert this uv to ndc based on camIndex
				const glm::vec2 equivNDC = glm::vec2((uv.x - 0.5*camIndex)*4.f - 1.f, uv.y*2.f - 1.f);
				const float radius = glm::length(equivNDC - ndcCenter[camIndex]);

				if (radius < (1.f + NDCcenterOffset)) {
					if (radius > middleRegionRadius) {//middle region checkerboard 2x2
						if ( (((x - 1) & 0x3) == 0) && (((y - 1) & 0x3) == 0) //both divis by 4
					      || (((x - 3) & 0x3) == 0) && (((y - 3) & 0x3) == 0) ) { //shift the above patter right and down to get checker
							//set group of 4 to stencilMask. uv as it is resolves to lower right pixel
							radialDensityMask[camIndex][(y  )*width + x  ] = stencilMaskVal;//lowerright
							radialDensityMask[camIndex][(y-1)*width + x  ] = stencilMaskVal;//upperright
							radialDensityMask[camIndex][(y-1)*width + x-1] = stencilMaskVal;//upperleft
							radialDensityMask[camIndex][(y  )*width + x-1] = stencilMaskVal;//lowerleft
						}
					} else { //center region
						//set group of 4 to stencilMask. uv as it is resolves to lower right pixel
						radialDensityMask[camIndex][(y  )*width + x  ] = stencilMaskVal;//lowerright
						radialDensityMask[camIndex][(y-1)*width + x  ] = stencilMaskVal;//upperright
						radialDensityMask[camIndex][(y-1)*width + x-1] = stencilMaskVal;//upperleft
						radialDensityMask[camIndex][(y  )*width + x-1] = stencilMaskVal;//lowerleft
					}
				} 

			}//x pixel ID
		}//y pixel ID
	}//camIndex

	//XOR values to get result
	for (int y = 0; y <height; ++y) {
		for (int x = 0; x < width; ++x) {
			const int stencilIndex = (y*width + x);
			uint32_t xorResult = radialDensityMask[0][stencilIndex] ^ radialDensityMask[1][stencilIndex];
			//radialDensityMask[2][stencilIndex] = radialDensityMask[0][stencilIndex];
			radialDensityMask[2][stencilIndex] = xorResult;
		}
	}

	//stb write to an image to check it out
	const int NUM_CHANNELS = 4;
	uint8_t* rgb_image = (uint8_t*)malloc(width * height * NUM_CHANNELS);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const int stencilIndex = (y*width + x);
			const int redindex = NUM_CHANNELS * stencilIndex;
			const uint8_t colorVal = 255 * radialDensityMask[2][stencilIndex];
			rgb_image[redindex + 0] = colorVal;
			rgb_image[redindex + 1] = colorVal;
			rgb_image[redindex + 2] = colorVal;
			rgb_image[redindex + 3] = 255;

		}
	}
	std::string fileloc("radialStencilMask.bmp");
	std::cout << "\nWriting radialStencilMask image to "<< fileloc;
	stbi_write_bmp(fileloc.c_str(), width, height, NUM_CHANNELS, rgb_image);
	stbi_image_free(rgb_image);
}