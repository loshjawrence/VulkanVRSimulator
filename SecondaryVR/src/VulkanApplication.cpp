#pragma once
#include "VulkanApplication.h"

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN


#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
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
	const int num = 2;
	const int numMeshesPerStride = 2;
	std::vector< std::tuple<std::string, int, glm::mat4> > defaultScene(num);
	for (int i = 0; i < num/numMeshesPerStride; i += numMeshesPerStride) {
		const float x = static_cast<float>(rng.nextUInt(3));
		const float y = static_cast<float>(rng.nextUInt(3));
		const float z = static_cast<float>(rng.nextUInt(3));
		defaultScene[i] = { std::string("res/objects/rock/rock.obj"), 1,
			glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.0f)),glm::vec3(x, y, z)) };
		defaultScene[i+1] = { std::string("res/objects/cube.obj"), 1,
			glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(1.0f)),glm::vec3(y, z, x)) };
		//defaultScene[i] = { std::string("res/objects/buddha.obj"), 1,//Largest that works
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(5.0f)),glm::vec3(y, z, x)) };
		//defaultScene[i] = { std::string("res/objects/nanosuit/nanosuit.obj"), 1,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.1f)),glm::vec3(x, y, z)) };
		//defaultScene[i+1] = { std::string("res/objects/cryteksponza/sponza.obj"), 0,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.005f)),glm::vec3(x, y, z)) };
		//defaultScene[i] = { std::string("res/objects/dabrovicsponza/sponza.obj"), 0,
		//defaultScene[i+1] = { std::string("res/objects/sibenikcathedral/sibenik.obj"), 0,
		//	glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.05f)),glm::vec3(x, y, z)) };
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
	contextInfo = VulkanContextInfo(window);
	VulkanApplication::setupDebugCallback();
	
	//describes input and output attachments and how subpasses relate to one another
	forwardRenderPass = VulkanRenderPass(contextInfo);

	//really just for initializing VulkanDescriptor::layoutTypes 
	forwardDescriptor = VulkanDescriptor(contextInfo);
	//VulkanDescriptor heyforwardDescriptor = VulkanDescriptor(contextInfo);

	contextInfo.createSwapChainFramebuffers(forwardRenderPass.renderPass);

	//setup forward pipelines from our forward shaders
	createPipelines();
	VulkanBuffer::createUniformBuffer(contextInfo, sizeof(UniformBufferObject), uniformBuffer, uniformBufferMemory);

	loadModels();

	createSemaphores();
	allocateCommandBuffers();
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

	std::vector<VkSemaphore> waitSemaphores = { imageAvailableSemaphore };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	updateUniformBuffer();
	////////////////
	//// RECORD ////
	////////////////
	//PRIMARY FILLED WITH HOMOGENOUS SECONDARY
	//VkCommandBufferInheritanceInfo inheritanceInfo = {};
	//beginRecordingPrimary(inheritanceInfo, imageIndex);
	//for (Model& model : models) {
	//	//record command buffers for visible objects
	//	for (Mesh& mesh : model.mMeshes) {
	//	    //TODO: the pipeline selection is wrong
	//		const uint32_t index = getForwardPipelineIndexFromTextureMapFlags(mesh.descriptor.textureMapFlags);
	//		forwardPipelines[index].recordCommandBufferSecondary(
	//			inheritanceInfo, imageIndex, contextInfo, model, mesh, camera.vrmode);
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




	//Primary bufer recorded directly with unsorted pipelines
	beginRecordingPrimary(imageIndex);
	for (Model& model : models) {
		//TODO: record only visible meshes
		for (Mesh& mesh : model.mMeshes) {
	      //TODO: the pipeline selection is wrong
			const uint32_t index = getForwardPipelineIndexFromTextureMapFlags(mesh.descriptor.textureMapFlags);
			forwardPipelines[index].recordCommandBufferPrimary(
				primaryForwardCommandBuffers[imageIndex], imageIndex, contextInfo, model, mesh, camera.vrmode);
		}
	}
	endRecordingPrimary(imageIndex);


	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = &waitSemaphores[0];
	submitInfo.pWaitDstStageMask = &waitStages[0];

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &primaryForwardCommandBuffers[imageIndex];

	std::vector<VkSemaphore> signalSemaphores = { forwardRenderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	submitInfo.pSignalSemaphores = &signalSemaphores[0];

	if (vkQueueSubmit(contextInfo.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to submit draw command buffer!";
		throw std::runtime_error(ss.str());
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = signalSemaphores.size();
	presentInfo.pWaitSemaphores = &signalSemaphores[0];

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
	inheritanceInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	inheritanceInfo.renderPass = forwardRenderPass.renderPass;
	inheritanceInfo.occlusionQueryEnable = VK_FALSE;
	inheritanceInfo.pipelineStatistics = 0;


	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(primaryForwardCommandBuffers[imageIndex], &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = forwardRenderPass.renderPass;
	renderPassInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = contextInfo.swapChainExtent;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

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
	renderPassInfo.renderPass = forwardRenderPass.renderPass;
	renderPassInfo.framebuffer = contextInfo.swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = contextInfo.swapChainExtent;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

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
	ubo.view[0] = camera.view[0];
	ubo.view[1] = camera.view[1];
	ubo.proj = camera.proj;
	ubo.proj[1][1] *= -1;//need for correct z-buffer order
	ubo.time = time;

	void* data;
	vkMapMemory(contextInfo.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(contextInfo.device, uniformBufferMemory);
}

void VulkanApplication::allocateCommandBuffers() {
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
			camera.processKeyboardAndUpdateView(MovementDirection::FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.processKeyboardAndUpdateView(MovementDirection::BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.processKeyboardAndUpdateView(MovementDirection::LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.processKeyboardAndUpdateView(MovementDirection::RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			camera.processKeyboardAndUpdateView(MovementDirection::UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			camera.processKeyboardAndUpdateView(MovementDirection::DOWN, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
			camera.updateVrModeAndCameras();
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

void VulkanApplication::destroyPipelines() {
	for (auto& pipeline : forwardPipelines) {
		pipeline.destroyVulkanPipeline(contextInfo);
	}
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
			forwardRenderPass, contextInfo, &(VulkanDescriptor::layoutTypes[numImageSamplers]));
	}

	//post process pipelines
	//postProcessPipelines.reserve(allShaders_PostProcessPipeline.size());
	//for (uint32_t i = 0; i < allShaders_PostProcessPipeline.size(); ++i) {
	//	const uint32_t numImageSamplers = allShaders_PostProcessPipeline[i].second;
	//	postProcessPipelines[i] = PostProcessPipeline(allShaders_PostProcessPipeline[i].first,
	//		forwardRenderPass, contextInfo, &(VulkanDescriptor::postProcessLayoutTypes[numImageSamplers-1]));
	//}

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

	forwardRenderPass.destroyRenderPass(contextInfo);

	contextInfo.destroyVulkanSwapChain();
}


void VulkanApplication::recreateSwapChain() {
	vkDeviceWaitIdle(contextInfo.device);

	cleanupSwapChain();

	contextInfo.createSwapChain(window);
	contextInfo.createSwapChainImageViews();


	forwardRenderPass.createRenderPass(contextInfo);
	forwardRenderPass.createRenderPassPostProcess(contextInfo);
	forwardRenderPass.createRenderPassPostProcessPresent(contextInfo);

	//NEW
	createPipelines();

	contextInfo.createDepthImage();

	contextInfo.createSwapChainFramebuffers(forwardRenderPass.renderPass);

	//update camera
	camera.updateDimensions(contextInfo.swapChainExtent);
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

	window = glfwCreateWindow(camera.width, camera.height, "VulkanVR", nullptr, nullptr);
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

	app->camera.processMouseAndUpdateView(xoffset, yoffset);
}

void VulkanApplication::GLFW_MouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
}

void VulkanApplication::GLFW_KeyCallback(GLFWwindow * window, int key, int scanmode, int action, int mods) {
}

void VulkanApplication::GLFW_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
	app->camera.processScrollAndUpdateView(yoffset);
}

