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
#include <set>
#include <unordered_map>

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


VulkanApplication::VulkanApplication() {
}


VulkanApplication::~VulkanApplication() {
}

void VulkanApplication::run() {
	initWindow();
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
	std::cout << "callbacks initialized" << std::endl;

}

void VulkanApplication::initVulkan() {
	contextInfo = VulkanContextInfo(window);
	VulkanApplication::setupDebugCallback();
	forwardRenderPass = VulkanRenderPass(contextInfo);
	forwardDescriptor = VulkanDescriptor(contextInfo);
	forwardPipeline = VulkanGraphicsPipeline(allShaders_DefaultPipeline[0],
		forwardRenderPass, contextInfo, &(forwardDescriptor.descriptorSetLayout));


	//the cookbook says framebuffers represent image subresources that correspond to renderpass attachments(input attachments and render targets)
	//add a framebuffer component to VulkanRenderPass?
	//keep swapchain related things with the swapchain
	contextInfo.createSwapChainFramebuffers(forwardRenderPass.renderPass);

	VkExtent2D defaultextent; defaultextent.width = 0; defaultextent.height = 0;

	VulkanImage modelTexture = VulkanImage(IMAGETYPE::TEXTURE, defaultextent, VK_FORMAT_R8G8B8A8_UNORM,
		contextInfo, contextInfo.graphicsCommandPools[0], std::string(TEXTURE_PATH));

	VulkanApplication::loadModel();
	VulkanBuffer::createVertexBuffer(contextInfo, contextInfo.graphicsCommandPools[0], vertices, vertexBuffer, vertexBufferMemory);
	VulkanBuffer::createIndexBuffer(contextInfo, contextInfo.graphicsCommandPools[0], indices, indexBuffer, indexBufferMemory);
	VulkanBuffer::createUniformBuffer(contextInfo, sizeof(UniformBufferObject), uniformBuffer, uniformBufferMemory);

	forwardDescriptor.createDescriptorSet(contextInfo, uniformBuffer, sizeof(UniformBufferObject), modelTexture.imageView, modelTexture.sampler);

	forwardPipeline.createCommandBuffers(contextInfo, forwardRenderPass, vertexBuffer, indexBuffer, indices, forwardDescriptor);
	forwardPipeline.createSemaphores(contextInfo);
}

void VulkanApplication::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		processInputAndUpdateFPS();
		updateUniformBuffer();
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
	modelTexture.destroyVulkanImage(contextInfo);

	//clean up descriptor
	forwardDescriptor.destroyVulkanDescriptor(contextInfo);

	//TODO: add to Camera clean up ubo
	vkDestroyBuffer(contextInfo.device, uniformBuffer, nullptr);
	vkFreeMemory(contextInfo.device, uniformBufferMemory, nullptr);

	//TODO: add destroy buffers to Mesh, clean up ibo, vbo
	vkDestroyBuffer(contextInfo.device, indexBuffer, nullptr);
	vkFreeMemory(contextInfo.device, indexBufferMemory, nullptr);
	vkDestroyBuffer(contextInfo.device, vertexBuffer, nullptr);
	vkFreeMemory(contextInfo.device, vertexBufferMemory, nullptr);

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

	forwardPipeline.createCommandBuffers(contextInfo, forwardRenderPass, vertexBuffer, indexBuffer, indices, forwardDescriptor);

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


void VulkanApplication::loadModel() {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void VulkanApplication::updateUniformBuffer() {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.proj = glm::perspective(glm::radians(45.0f), contextInfo.swapChainExtent.width / (float)contextInfo.swapChainExtent.height, 0.1f, 10.0f);
	ubo.view = camera.view;
	ubo.proj = camera.proj;
	//ubo.proj[1][1] *= -1;//if doing opengl calculations for camera, need this to account for vulkan, screen origin starts upper left instead of bottom right

	void* data;
	vkMapMemory(contextInfo.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(contextInfo.device, uniformBufferMemory);
}


void VulkanApplication::drawFrame() {
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(contextInfo.device, contextInfo.swapChain, std::numeric_limits<uint64_t>::max(), forwardPipeline.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { forwardPipeline.imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &forwardPipeline.commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { forwardPipeline.renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(contextInfo.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { contextInfo.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(contextInfo.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	vkQueueWaitIdle(contextInfo.presentQueue);
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
	//float yoffset = app->lastY - ypos; // reversed since y-coordinates go from bottom to top for OpenGL
	float yoffset = ypos - app->lastY; // vulkan

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

