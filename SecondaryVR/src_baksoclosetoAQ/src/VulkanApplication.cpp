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
	for (uint32_t i = 0; i < num/numMeshesPerStride; i += numMeshesPerStride) {
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

//		defaultScene[i] = { std::string("res/objects/cyborg/cyborg.obj"), 1,
//			glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.0f)),glm::vec3(0, 0, 0)) };
//		defaultScene[i] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
//			glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.25f)),glm::vec3(0, 0, 0)) };
		//defaultScene[i+1] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x, y, z)) };
		//defaultScene[i+2] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x+1, y, z)) };
		//defaultScene[i+3] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x+2, y, z)) };
//		defaultScene[i] = { std::string("res/objects/cryteksponza/sponza.obj"), 0,
//			glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.005f)),glm::vec3(x, y, z)) };
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

	//context info holds vulkan things like instance, phys and logical device, swap chain info, depthImage, command pools and queues
	//contextInfo = VulkanContextInfo(window,std::string("radialStencilMask.bmp"));
	contextInfo = VulkanContextInfo(window);
	VulkanApplication::setupDebugCallback();

	
	//describes input and output attachments and how subpasses relate to one another
	allRenderPasses = VulkanRenderPass(contextInfo);

	//really just for initializing static VulkanDescriptor::"pipeline"LayoutTypes vectors
	VulkanDescriptor justInit = VulkanDescriptor(contextInfo);

	//contextInfo.createSwapChainFramebuffers(allRenderPasses.renderPass);
	contextInfo.createSwapChainFramebuffers(allRenderPasses.renderPassPostProcessPresent);

	createPPMeshes();

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
	//PRIMARY FILLED WITH HOMOGENOUS SECONDARY COMMAND BUFFERS
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




	////PRIMARY BUFER RECORDED DIRECTLY WITH UNSORTED PIPELINES
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

	int k = contextInfo.camera.vrmode ? contextInfo.camera.qualityIndex : 0;

	for (auto& pipeline : postProcessPipelines) {
		postProcessSubmitInfo.pWaitSemaphores = &postProcessWaitSemaphores[0];
		postProcessSubmitInfo.pCommandBuffers = &pipeline.commandBuffers[pipeline.isPresent ? 0 : k][imageIndex];
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
	const int k = contextInfo.camera.vrmode ? contextInfo.camera.qualityIndex : 0;

	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = NULL;
	//inheritanceInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	inheritanceInfo.framebuffer = forwardPipelinesFramebuffers[k][imageIndex];
	inheritanceInfo.renderPass = (contextInfo.camera.vrmode && useStencil) ? 
		allRenderPasses.renderPassStencilLoading : allRenderPasses.renderPass;
	inheritanceInfo.occlusionQueryEnable = VK_FALSE;
	inheritanceInfo.pipelineStatistics = 0;


	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(primaryForwardCommandBuffers[imageIndex], &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = inheritanceInfo.renderPass;
	//renderPassInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	renderPassInfo.framebuffer = forwardPipelinesFramebuffers[k][imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
//	renderPassInfo.renderArea.extent = contextInfo.swapChainExtent;
	VkExtent2D renderTargetExtent = contextInfo.camera.vrmode ? contextInfo.camera.renderTargetExtent[k] : contextInfo.camera.renderTargetExtentNoVR;
	renderPassInfo.renderArea.extent = renderTargetExtent;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	if(contextInfo.camera.vrmode && useStencil)
		clearValues[1].depthStencil = { 1.f };
	else
		clearValues[1].depthStencil = { 1.f, 1 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(primaryForwardCommandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void VulkanApplication::beginRecordingPrimary(const uint32_t imageIndex) {
	const int k = contextInfo.camera.vrmode ? contextInfo.camera.qualityIndex : 0;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(primaryForwardCommandBuffers[imageIndex], &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = (contextInfo.camera.vrmode && useStencil) ? 
		allRenderPasses.renderPassStencilLoading : allRenderPasses.renderPass;
	//renderPassInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	renderPassInfo.framebuffer = forwardPipelinesFramebuffers[k][imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	//renderPassInfo.renderArea.extent = contextInfo.swapChainExtent;
	VkExtent2D renderTargetExtent = contextInfo.camera.vrmode ? contextInfo.camera.renderTargetExtent[k] : contextInfo.camera.renderTargetExtentNoVR;
	renderPassInfo.renderArea.extent = renderTargetExtent;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	if(contextInfo.camera.vrmode && useStencil)
		clearValues[1].depthStencil = { 1.f };
	else
		clearValues[1].depthStencil = { 1.f, 1 };


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
	ubo.viewProj[0] = ubo.proj * contextInfo.camera.view[0];
	ubo.viewProj[1] = ubo.proj * contextInfo.camera.view[1];
	ubo.viewPos = glm::vec4(contextInfo.camera.camPos, 1.f);
	ubo.lightPos = glm::vec4(100.f, 100.f, 100.f, 1.f);
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
		contextInfo.camera.updateDimensions(contextInfo.swapChainExtent);
		recreateSwapChain();
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS && contextInfo.camera.vrmode && contextInfo.camera.qualityEnabled) {
		const bool increaseQuality = false;
		const int indexBefore = contextInfo.camera.qualityIndex;
		contextInfo.camera.updateQualitySettings(increaseQuality);
		contextInfo.camera.updateDimensions(contextInfo.swapChainExtent);
		const int indexAfter = contextInfo.camera.qualityIndex;
		if (indexBefore != indexAfter) {
			std::cout << "\nDecreased Quality to: " << contextInfo.camera.vrScalings[contextInfo.camera.qualityIndex];
			std::cout << "\nVR virtual Render Target Dim: " << contextInfo.camera.width*2.f << ", " << contextInfo.camera.height;
			recreateSwapChain();
		}
	}
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && contextInfo.camera.vrmode && contextInfo.camera.qualityEnabled) {
		const bool increaseQuality = true;
		const int indexBefore = contextInfo.camera.qualityIndex;
		contextInfo.camera.updateQualitySettings(increaseQuality);
		contextInfo.camera.updateDimensions(contextInfo.swapChainExtent);
		const int indexAfter = contextInfo.camera.qualityIndex;
		if (indexBefore != indexAfter) {
			std::cout << "\nIncreased Quality to: " << contextInfo.camera.vrScalings[contextInfo.camera.qualityIndex];
			std::cout << "\nVR virtual Render Target Dim: " << contextInfo.camera.width*2.f << ", " << contextInfo.camera.height;
			recreateSwapChain();
		}
	}
	
	//TODO:poor performance, for now just make it key toggle. to make this smooth you would 
	//need to make a set of render target images/framebuffers (pipelines?, at least pipeline image views and input descriptors(for pp pipleines)) 
	//and static command buffers (post process) for each setting
	//if (contextInfo.camera.vrmode && contextInfo.camera.qualityEnabled) {
	//	const float timeRatio = deltaTime*1000.f / contextInfo.camera.targetFrameTime_ms;
	//	if (timeRatio > 0.9f) { //turn down settings
	//		const bool increaseQuality = false;
	//		const int indexBefore = contextInfo.camera.qualityIndex;
	//		contextInfo.camera.updateQualitySettings(increaseQuality);
	//		contextInfo.camera.updateDimensions(contextInfo.swapChainExtent);
	//		const int indexAfter = contextInfo.camera.qualityIndex;
	//		if (indexBefore != indexAfter) {
	//			//std::cout << "\nDecreased Quality to: " << contextInfo.camera.vrScalings[contextInfo.camera.qualityIndex];
	//			//std::cout << "\nVR virtual Render Target Dim: " << contextInfo.camera.width*2.f << ", " << contextInfo.camera.height;
	//			recreateSwapChain();
	//		}
	//	} else if (timeRatio > 0.80) {//stay here
	//	} else { //turn up settings
	//		const bool increaseQuality = true;
	//		const int indexBefore = contextInfo.camera.qualityIndex;
	//		contextInfo.camera.updateQualitySettings(increaseQuality);
	//		contextInfo.camera.updateDimensions(contextInfo.swapChainExtent);
	//		const int indexAfter = contextInfo.camera.qualityIndex;
	//		if (indexBefore != indexAfter) {
	//			//std::cout << "\nIncreased Quality to: " << contextInfo.camera.vrScalings[contextInfo.camera.qualityIndex];
	//			//std::cout << "\nVR virtual Render Target Dim: " << contextInfo.camera.width*2.f << ", " << contextInfo.camera.height;
	//			recreateSwapChain();
	//		}
	//	}
	//}
}

void VulkanApplication::cleanup() {
	cleanupSwapChain();

	//TODO: add to Mesh cleanup texture image
	for (auto& model : models) {
		model.destroyVulkanHandles(contextInfo);
	}
	vkDestroyBuffer(contextInfo.device, uniformBuffer, nullptr);
	vkFreeMemory(contextInfo.device, uniformBufferMemory, nullptr);

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
	initForwardPipelinesVulkanImagesAndFramebuffers();

	for (uint32_t i = 0; i < allShaders_ForwardPipeline.size(); ++i) {
		int numImageSamplers = 0;
		for (uint32_t k = 1; k < VulkanDescriptor::MAX_IMAGESAMPLERS + 1; ++k) //the first bit is for HAS_NONE so we need to ignore that one
			numImageSamplers = (allShaders_ForwardPipeline[i].second & 1 << k) ? numImageSamplers + 1 : numImageSamplers;

		textureMapFlagsToForwardPipelineIndex[i] = allShaders_ForwardPipeline[i].second;//create the mapping based on shaders we have
		forwardPipelines[i] = VulkanGraphicsPipeline(allShaders_ForwardPipeline[i].first,
			allRenderPasses, contextInfo, &(VulkanDescriptor::layoutTypes[numImageSamplers]));
	}

	//post process pipelines
	postProcessPipelines.resize(allShaders_PostProcessPipeline.size());
	for (uint32_t i = 0; i < allShaders_PostProcessPipeline.size(); ++i) {
		const uint32_t numImageSamplers = std::get<1>(allShaders_PostProcessPipeline[i]);
		const std::vector<std::string>& shaderPaths = std::get<0>(allShaders_PostProcessPipeline[i]);

		//need an mapping from numimagesamplers to layouttypes index
		if (i != allShaders_PostProcessPipeline.size() - 1) { //if not last output format should be 16F
			postProcessPipelines[i] = PostProcessPipeline(shaderPaths,
				allRenderPasses, contextInfo, &(VulkanDescriptor::postProcessLayoutTypes[numImageSamplers - 1]), false);
		} else { //last one, outputImage should be swapchain format
			postProcessPipelines[i] = PostProcessPipeline(shaderPaths,
				allRenderPasses, contextInfo, &(VulkanDescriptor::postProcessLayoutTypes[numImageSamplers - 1]), true);
		}
	}

	//each pp needs inputdescriptor set ofprevious stage
	postProcessPipelines[0].createInputDescriptors(contextInfo, forwardPipelinesVulkanImages);
	for (uint32_t i = 1; i < postProcessPipelines.size(); ++i) {
		//TODO: second are should be determined from tuple element in allShaders_PostProcessPipeline specifying which stage feeds it
		postProcessPipelines[i].createInputDescriptors(contextInfo, postProcessPipelines[i-1].outputImages);
	}

	//create the static command buffers(no dynamic input for post processing)
	for (uint32_t i = 0; i < allShaders_PostProcessPipeline.size(); ++i) {
		//need an mapping from numimagesamplers to layouttypes index
		if (i != allShaders_PostProcessPipeline.size() - 1) { //if not last output format should be 16F
			postProcessPipelines[i].createStaticCommandBuffers(contextInfo, allRenderPasses, {ndcTriangle, ndcTriangle});
		} else { //last one, outputImage should be swapchain format
			postProcessPipelines[i].createStaticCommandBuffers(contextInfo, allRenderPasses, {ndcTriangle, ndcTriangle});
			//postProcessPipelines[i].createStaticCommandBuffers(contextInfo, allRenderPasses, {ndcBarrelMesh[0], ndcBarrelMesh[1]});
			//postProcessPipelines[i].createStaticCommandBuffers(contextInfo, allRenderPasses, {ndcBarrelMesh_PreCalc[0], ndcBarrelMesh_PreCalc[1]});
		}
	}


}

void VulkanApplication::initForwardPipelinesVulkanImagesAndFramebuffers() {
	const int numForwardImages = contextInfo.camera.vrmode ? contextInfo.camera.numQualitySettings : 1;

	forwardPipelinesVulkanImages.resize(numForwardImages);
	forwardPipelinesFramebuffers.resize(numForwardImages);


	for (int k = 0; k < numForwardImages; ++k) {
		forwardPipelinesVulkanImages[k].resize(contextInfo.swapChainImages.size());
		forwardPipelinesFramebuffers[k].resize(contextInfo.swapChainImages.size());
		VkExtent2D renderTargetExtent = contextInfo.camera.vrmode ?
			contextInfo.camera.renderTargetExtent[k] : contextInfo.camera.renderTargetExtentNoVR;
		const int j = contextInfo.camera.vrmode ? k : 0;

		for (uint32_t i = 0; i < contextInfo.swapChainImages.size(); ++i) {
			forwardPipelinesVulkanImages[k][i] = VulkanImage(IMAGETYPE::COLOR_ATTACHMENT, renderTargetExtent, VK_FORMAT_R16G16B16A16_SFLOAT, contextInfo);
		}

		for (uint32_t i = 0; i < contextInfo.swapChainImages.size(); ++i) {
			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = NULL;

			std::vector<VkImageView> attachments = { forwardPipelinesVulkanImages[k][i].imageView, contextInfo.depthImage[j].imageView };
			framebufferCreateInfo.renderPass = (contextInfo.camera.vrmode && useStencil) ?
				allRenderPasses.renderPassStencilLoading : allRenderPasses.renderPass;
			framebufferCreateInfo.pAttachments = attachments.data();
			framebufferCreateInfo.attachmentCount = attachments.size();
			framebufferCreateInfo.width = renderTargetExtent.width;
			framebufferCreateInfo.height = renderTargetExtent.height;
			framebufferCreateInfo.layers = 1;

			if (vkCreateFramebuffer(contextInfo.device, &framebufferCreateInfo, nullptr, &forwardPipelinesFramebuffers[k][i]) != VK_SUCCESS) {
				std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to create framebuffer!";
				throw std::runtime_error(ss.str());
			}
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
	for (int i = 0; i < contextInfo.numDepthImages; ++i) {
		contextInfo.depthImage[i].destroyVulkanImage(contextInfo);
	}

	destroyPipelines();
	destroyOffScreenRenderTargets();
	freeGlobalCommandBuffers();

	allRenderPasses.destroyRenderPasses(contextInfo);//don't think this is needed for resize but needed for final cleanup

	contextInfo.destroyVulkanSwapChain();
}

void VulkanApplication::destroyOffScreenRenderTargets() {

	for (int i = 0; i < forwardPipelinesVulkanImages.size(); ++i) {
		for (auto& image : forwardPipelinesVulkanImages[i]) {
			image.destroyVulkanImage(contextInfo);
			//depthImage destroyed before this call
		}
	}

	//dont need to do the last one since it refers to the swap chain
	for (uint32_t i = 0; i < postProcessPipelines.size() - 1; ++i) {
		for (int k = 0; k < postProcessPipelines[i].outputImages.size(); ++k) {
			for (auto& image : postProcessPipelines[i].outputImages[k]) {
				image.destroyVulkanImage(contextInfo);
			}
		}
	}
}

void VulkanApplication::freeGlobalCommandBuffers() {
	for(uint32_t i = 0; i < primaryForwardCommandBuffers.size(); ++i) {
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
	allocateGlobalCommandBuffers();//primary for forward render pass

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

	window = glfwCreateWindow(hmdWidth, hmdHeight, "VulkanVR", nullptr, nullptr);
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

void VulkanApplication::createPPMeshes() {
	ndcTriangle = Mesh(contextInfo, MESHTYPE::NDCTRIANGLE);//ndc triangle for post processing
	ndcBarrelMesh[0] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH, 0);
	ndcBarrelMesh[1] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH, 1);
	ndcBarrelMesh_PreCalc[0] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH_PRECALC, 0);
	ndcBarrelMesh_PreCalc[1] = Mesh(contextInfo, MESHTYPE::NDCBARRELMESH_PRECALC, 1);
}
