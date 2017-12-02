#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>


//Vulkan
#include "Vertex.h"
#include "VulkanContextInfo.h"
#include "VulkanRenderPass.h"
#include "VulkanDescriptor.h"
#include "VulkanGraphicsPipeline.h"
#include "PostProcessPipeline.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "Model.h"
#include "../dependencies/pcg32.h"

#include "Camera.h"

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}

struct UniformBufferObject {
	glm::mat4 view[2];
    glm::mat4 proj;
	float time;
};

class VulkanApplication {
public:
	VulkanApplication();
	~VulkanApplication();

	void run();

private:
    GLFWwindow* window;

    VkDebugReportCallbackEXT callback;

	//RNG
	pcg32 rng = pcg32(17);

	std::vector<Model> models;

	//submitting a vector of primary buffers didn't seem to work, let try recording all in one:
	//beginbuf->beginpass->record all->endpass->endbuf
	std::vector<uint32_t> textureMapFlagsToForwardPipelineIndex;//element is textureMapFlags index is pipeline index

	//commandPools
	//generally only need 1, but if you want to do multithreaded command recording each thread needs its own pool
	std::vector<VkCommandPool> graphicsCommandPools;
	std::vector<VkCommandPool> computeCommandPools;
	std::vector<VkCommandBuffer> primaryForwardCommandBuffers;

	
	//camera
	bool firstmouse = true;
	float lastX = hmdWidth / 2.f;
	float lastY = hmdHeight / 2.f;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;


	//Vulkan components
	VulkanContextInfo contextInfo;
	std::vector<VulkanGraphicsPipeline> forwardPipelines;
	std::vector<VkFramebuffer> forwardPipelinesFramebuffers;
	std::vector<VulkanImage> forwardPipelinesVulkanImages;
	VulkanRenderPass allRenderPasses;
	std::vector<PostProcessPipeline> postProcessPipelines;
	Mesh ndcTriangle;
	Mesh ndcBarrelMesh[2];
	Mesh ndcBarrelMesh_PreCalc[2];//0 left, 1 right

	//used for fps tracker
	double oldtime = 0.f;
	double currenttime = 0.f;
	int fps = 0;
	int fpstracker = 0;
	
	//used for physical movement
	float time = 0.f;
	float deltaTime = 0.f;
	float lastFrame = 0.f;

	//semphores for communication bewteen various stages
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore forwardRenderFinishedSemaphore;

private:
	void createRadialStencilMask();
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	void updateFPS();
	void processInputAndUpdateFPS();

	void loadModels();
	void updateUniformBuffer();
	void initForwardPipelinesVulkanImagesAndFramebuffers();

	//drawing
	void drawFrame();

	//window resized need to recreate swap chain
	void cleanupSwapChain();
	void recreateSwapChain();

	//helper
	uint32_t getForwardPipelineIndexFromTextureMapFlags(const uint32_t textureMapFlags);

	//callbacks
	void setupDebugCallback();
	static void onWindowResized(GLFWwindow* window, int width, int height);
	void DestroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
	VkResult CreateDebugReportCallbackEXT(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, 
		uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);

	void allocateGlobalCommandBuffers();
	void addGraphicsCommandPool(const int num);
	void beginRecordingPrimary(const uint32_t imageIndex);
	void beginRecordingPrimary(VkCommandBufferInheritanceInfo& inheritanceInfo, const uint32_t imageIndex);
	void endRecordingPrimary(const uint32_t imageIndex);

	void createSemaphores();
	void createPipelines();
	void destroyPipelines();
	void destroyOffScreenRenderTargets();
	void freeGlobalCommandBuffers();
	void destroyPipelinesSemaphores();

	static void VulkanApplication::GLFW_MousePosCallback(GLFWwindow * window, double xpos, double ypos);
	static void GLFW_MouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
	static void GLFW_KeyCallback(GLFWwindow * window, int key, int scanmode, int action, int mods);
	static void GLFW_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};

