#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "GlobalSettings.h"
#include "Utils.h"
#include "Vertex.h"
#include "VulkanContextInfo.h"
#include "VulkanRenderPass.h"
#include "VulkanDescriptor.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"


#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <array>
#include <set>
#include <unordered_map>

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkDebugReportCallbackEXT callback;

	//should belong in Model or Mesh
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

	//should belong in camera?
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;


	VulkanContextInfo contextInfo;
	VulkanGraphicsPipeline forwardPipeline;
	VulkanRenderPass forwardRenderPass;
	VulkanDescriptor forwardDescriptor;
	VulkanImage vulkanDepthImage;

	//should belong in Model or Mesh
	VulkanImage modelTexture;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetWindowSizeCallback(window, HelloTriangleApplication::onWindowResized);
    }

    void initVulkan() {
		contextInfo = VulkanContextInfo(window);
        setupDebugCallback();
		forwardRenderPass = VulkanRenderPass(contextInfo);
		forwardDescriptor = VulkanDescriptor(contextInfo);
		forwardPipeline = VulkanGraphicsPipeline(allShaders_DefaultPipeline[0], 
			forwardRenderPass, contextInfo, &(forwardDescriptor.descriptorSetLayout));


		//TODO: put this inside VulkanContextInfo?
		//contextInfo.createDepthImage();
		vulkanDepthImage = VulkanImage(IMAGETYPE::DEPTH, contextInfo.swapChainExtent, contextInfo.depthFormat,
			contextInfo, contextInfo.graphicsCommandPools[0]);

		//the cookbook says framebuffers represent image subresources that correspond to renderpass attachments(input attachments and render targets)
		//add a framebuffer compoent to VulkanRenderPass?
		contextInfo.createSwapChainFramebuffers(vulkanDepthImage.imageView, forwardRenderPass.renderPass);

		VkExtent2D defaultextent; defaultextent.width = 0; defaultextent.height = 0;

		VulkanImage modelTexture = VulkanImage(IMAGETYPE::TEXTURE, defaultextent, VK_FORMAT_R8G8B8A8_UNORM,
			contextInfo, contextInfo.graphicsCommandPools[0], std::string(TEXTURE_PATH));

        loadModel();
		VulkanBuffer::createVertexBuffer(contextInfo, contextInfo.graphicsCommandPools[0], vertices, vertexBuffer, vertexBufferMemory);
		VulkanBuffer::createIndexBuffer(contextInfo, contextInfo.graphicsCommandPools[0], indices, indexBuffer, indexBufferMemory);
		VulkanBuffer::createUniformBuffer(contextInfo, sizeof(UniformBufferObject), uniformBuffer, uniformBufferMemory);

		forwardDescriptor.createDescriptorSet(contextInfo, uniformBuffer, sizeof(UniformBufferObject), modelTexture.imageView, modelTexture.sampler);

		forwardPipeline.createCommandBuffers(contextInfo, forwardRenderPass, vertexBuffer, indexBuffer, indices, forwardDescriptor);
		forwardPipeline.createSemaphores(contextInfo);
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            updateUniformBuffer();
            drawFrame();
        }
        vkDeviceWaitIdle(contextInfo.device);
    }


    void cleanup() {
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
        contextInfo.DestroyDebugReportCallbackEXT(callback, nullptr);
		contextInfo.destroySurface();
		contextInfo.destroyInstance();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static void onWindowResized(GLFWwindow* window, int width, int height) {
        if (width == 0 || height == 0) return;

        HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->recreateSwapChain();
    }

    void cleanupSwapChain() {
        vkDestroyImageView(contextInfo.device, vulkanDepthImage.imageView, nullptr);
        vkDestroyImage(contextInfo.device, vulkanDepthImage.image, nullptr);
        vkFreeMemory(contextInfo.device, vulkanDepthImage.imageMemory, nullptr);

		contextInfo.destroySwapChainFramebuffers();

		forwardPipeline.freeCommandBuffers(contextInfo);

		forwardPipeline.destroyPipeline(contextInfo);
		forwardPipeline.destroyPipelineLayout(contextInfo);
		forwardRenderPass.destroyRenderPass(contextInfo);

		contextInfo.destroySwapChainImageViews();

		contextInfo.destroySwapChain();
    }

    void recreateSwapChain() {
        vkDeviceWaitIdle(contextInfo.device);

        cleanupSwapChain();
		contextInfo.createSwapChain(window);

		contextInfo.createSwapChainImageViews();


		forwardRenderPass.createRenderPass(contextInfo);
        
		forwardPipeline.createGraphicsPipeline(forwardRenderPass, contextInfo, &forwardDescriptor.descriptorSetLayout);

		vulkanDepthImage = VulkanImage(IMAGETYPE::DEPTH, contextInfo.swapChainExtent, contextInfo.depthFormat,
			contextInfo, contextInfo.graphicsCommandPools[0]);

		contextInfo.createSwapChainFramebuffers(vulkanDepthImage.imageView, forwardRenderPass.renderPass);

		forwardPipeline.createCommandBuffers(contextInfo, forwardRenderPass, vertexBuffer, indexBuffer, indices, forwardDescriptor);

    }


    void setupDebugCallback() {
        if (!enableValidationLayers) return;

        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        createInfo.pfnCallback = debugCallback;

        if (contextInfo.CreateDebugReportCallbackEXT(&createInfo, nullptr, &callback) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug callback!");
        }
    }


    void loadModel() {
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

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void updateUniformBuffer() {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

        UniformBufferObject ubo = {};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), contextInfo.swapChainExtent.width / (float) contextInfo.swapChainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        void* data;
        vkMapMemory(contextInfo.device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(contextInfo.device, uniformBufferMemory);
    }

    void drawFrame() {
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

        VkSemaphore waitSemaphores[] = {forwardPipeline.imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &forwardPipeline.commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {forwardPipeline.renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(contextInfo.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {contextInfo.swapChain};
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


    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
        std::cerr << "validation layer: " << msg << std::endl;

        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}