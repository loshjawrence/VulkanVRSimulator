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
	const int num = 10;
	std::vector< std::tuple<std::string, int, glm::mat4> > defaultScene(num);
	for (int i = 0; i < num; ++i) {
		const float x = rng.nextUInt(15);
		const float y = rng.nextUInt(15);
		const float z = rng.nextUInt(15);
		//defaultScene[i] = { std::string("res/objects/rock/rock.obj"), 1,
		defaultScene[i] = { std::string("res/objects/cube.obj"), 1,
			glm::translate(glm::mat4(1.f),glm::vec3(x, y, z)) };
	}

	for (auto& modelinfo : defaultScene) {//defaultScene in GlobalSettings.h
		models.push_back(Model(std::get<0>(modelinfo), std::get<1>(modelinfo), std::get<2>(modelinfo),
			contextInfo, uniformBuffer, uniformBufferMemory, sizeof(UniformBufferObject)));
		//models.push_back(Model(std::get<0>(modelinfo), std::get<1>(modelinfo), std::get<2>(modelinfo), contextInfo));
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

	//the cookbook says framebuffers represent image subresources that correspond to renderpass attachments(input attachments and render targets)
	//add a framebuffer component to VulkanRenderPass?
	//keep swapchain related things with the swapchain

	contextInfo.createSwapChainFramebuffers(forwardRenderPass.renderPass);

	//forwardPipeline = VulkanGraphicsPipeline(allShaders_ForwardPipeline[1],
	//	forwardRenderPass, contextInfo, &(VulkanDescriptor::layoutTypes[1]));
	for (int i = 0; i < allShaders_ForwardPipeline.size(); ++i) {
		if (i >= 2) {//height
			//TODO: make the pack of shader info include descriptor type and make a descriptor type to layoutType conversion function
			forwardPipelines.push_back(VulkanGraphicsPipeline(allShaders_ForwardPipeline[i],
				forwardRenderPass, contextInfo, &(VulkanDescriptor::layoutTypes[i - 1])));
		} else {
			forwardPipelines.push_back(VulkanGraphicsPipeline(allShaders_ForwardPipeline[i],
				forwardRenderPass, contextInfo, &(VulkanDescriptor::layoutTypes[i])));
		}

	}

	VulkanBuffer::createUniformBuffer(contextInfo, sizeof(UniformBufferObject), uniformBuffer, uniformBufferMemory);

	loadModels();

	createSemaphores();
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

	//std::vector<VkSemaphore> waitSemaphores = { imageAvailableSemaphore };
	//finished signals for each stage, successive stages need to wait on the previous finish
	//std::vector<VkSemaphore> signalSemaphores = { forwardRenderFinishedSemaphore };

	//NEW
	std::vector<VkSemaphore> waitSemaphores;
	std::vector<VkSemaphore> signalSemaphores;
	std::vector<VkPipelineStageFlags> waitStages;
	////////////////
	//// RECORD ////
	////////////////
	updateUniformBuffer();
	for (Model& model : models) {
	//record command buffers for visible objects
		//updateUniformBuffer(model);//test against push constant
		for (Mesh& mesh : model.mMeshes) {
			//forwardPipeline.recordCommandBufferTEST(imageIndex, contextInfo, forwardRenderPass, model, mesh, time);

			//NEW
			forwardPipelines[mesh.descriptor.numImageSamplers].recordCommandBufferTEST(imageIndex, contextInfo,
				forwardRenderPass, model, mesh, time);
		}
	}
	//forwardPipeline.endRecording(imageIndex);

	//NEW
	std::vector<VkCommandBuffer> forwardCommandBuffers;
	for (int i = 0; i < forwardPipelines.size(); ++i) {
		if (forwardPipelines[i].endRecording(imageIndex)) {
			forwardCommandBuffers.push_back(forwardPipelines[i].commandBuffers[imageIndex]);
			waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			waitSemaphores.push_back(imageAvailableSemaphore);
			signalSemaphores.push_back(forwardPipelines[i].renderFinishedSemaphore);
		}
	}

	//forwardPipeline.recordCommandBuffer(imageIndex, contextInfo, forwardRenderPass, vertexBuffer, indexBuffer, indices, models[0].mMeshes[0].descriptor);
	/////////////////
	//// SUBMIT /////
	/////////////////
	//VkSubmitInfo submitInfo = {};
	//submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	//VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	//submitInfo.waitSemaphoreCount = 1;
	//submitInfo.pWaitSemaphores = &waitSemaphores[0];
	//submitInfo.pWaitDstStageMask = waitStages;

	//submitInfo.commandBufferCount = 1;
	//submitInfo.pCommandBuffers = &forwardPipeline.commandBuffers[imageIndex];

	//submitInfo.signalSemaphoreCount = 1;
	//submitInfo.pSignalSemaphores = &signalSemaphores[0];

	//NEW
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = &waitSemaphores[0];
	submitInfo.pWaitDstStageMask = &waitStages[0];

	submitInfo.commandBufferCount = forwardCommandBuffers.size();
	submitInfo.pCommandBuffers = &forwardCommandBuffers[0];

	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	submitInfo.pSignalSemaphores = &signalSemaphores[0];

	if (vkQueueSubmit(contextInfo.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		std::stringstream ss; ss << "\n" << __LINE__ << ": " << __FILE__ << ": failed to submit draw command buffer!";
		throw std::runtime_error(ss.str());
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	//presentInfo.waitSemaphoreCount = 1;
	//presentInfo.pWaitSemaphores = &signalSemaphores.back();

	//NEW
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
	ubo.view = camera.view;
	ubo.proj = camera.proj;
	ubo.proj[1][1] *= -1;//need for correct z-buffer order

	void* data;
	vkMapMemory(contextInfo.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(contextInfo.device, uniformBufferMemory);
}

void VulkanApplication::updateUniformBuffer(const Model& model) {
	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(model.modelMatrix, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = camera.view;
	ubo.proj = camera.proj;
	ubo.proj[1][1] *= -1;//need for correct z-buffer order

	void* data;
	vkMapMemory(contextInfo.device, model.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(contextInfo.device, model.uniformBufferMemory);
}

void VulkanApplication::recordAndSubmitForwardRendering(const uint32_t imageIndex) {
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
}

void VulkanApplication::cleanup() {
	cleanupSwapChain();

	//TODO: add to Mesh cleanup texture image
	for (auto& model : models) {
		model.destroyVulkanHandles(contextInfo);
	}

	//clean up pipeline semaphores
	forwardPipeline.destroyPipelineSemaphores(contextInfo);

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


void VulkanApplication::cleanupSwapChain() {
	contextInfo.depthImage.destroyVulkanImage(contextInfo);

	forwardPipeline.destroyVulkanPipeline(contextInfo);

	forwardRenderPass.destroyRenderPass(contextInfo);

	contextInfo.destroyVulkanSwapChain();
}

void VulkanApplication::recreateSwapChain() {
	vkDeviceWaitIdle(contextInfo.device);

	cleanupSwapChain();

	contextInfo.createSwapChain(window);
	contextInfo.createSwapChainImageViews();


	forwardRenderPass.createRenderPass(contextInfo);

	forwardPipeline.createGraphicsPipeline(forwardRenderPass, contextInfo, &forwardDescriptor.descriptorSetLayout);

	contextInfo.createDepthImage();

	contextInfo.createSwapChainFramebuffers(forwardRenderPass.renderPass);

	//forwardPipeline.createCommandBuffers(contextInfo, forwardRenderPass, vertexBuffer, indexBuffer, indices, forwardDescriptor);

	//update camera
	camera.updateDimensionsAndUBO(contextInfo.swapChainExtent);
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

