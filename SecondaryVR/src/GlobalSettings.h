#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <tuple>
#include <string>


const uint32_t HAS_NONE		= 1 << 0; 
const uint32_t HAS_DIFFUSE	= 1 << 1; 
const uint32_t HAS_NOR		= 1 << 2; 
const uint32_t HAS_SPEC		= 1 << 3;
const uint32_t HAS_HEIGHT	= 1 << 4;
const std::vector< std::pair<std::vector<std::string>, uint32_t> > allShaders_ForwardPipeline =
{
	//No Tex shaders
	{{"src/shaders/forward.vert.spv",
	"src/shaders/forwardNoTex.frag.spv"},
	HAS_NONE},

	//forwardDiffuse shaders
	{{"src/shaders/forward.vert.spv",
	"src/shaders/forwardDiffuse.frag.spv"},
	HAS_DIFFUSE},

	//forwardNor shaders
	{{"src/shaders/forward.vert.spv",
	"src/shaders/forwardDiffuseNor.frag.spv"},
	HAS_NOR | HAS_DIFFUSE},

	//forwardHeight shaders
	{{"src/shaders/forward.vert.spv",
	"src/shaders/forwardDiffuseHeight.frag.spv"},
	HAS_HEIGHT | HAS_DIFFUSE},

	//forwardSpec shaders
	{{"src/shaders/forward.vert.spv",
	"src/shaders/forwardSpecNor.frag.spv"},
	HAS_SPEC | HAS_NOR | HAS_DIFFUSE},

	//forwardSpec shaders
	{{"src/shaders/forward.vert.spv",
	"src/shaders/forwardSpecHeight.frag.spv"},
	HAS_SPEC | HAS_HEIGHT | HAS_DIFFUSE},

	//forwardAll shaders (would a shader have both nor and height?)
	{{"src/shaders/forward.vert.spv",
	 "src/shaders/forwardAll.frag.spv"},
	HAS_SPEC | HAS_HEIGHT | HAS_NOR | HAS_DIFFUSE},
};


//name of shaders and number of input sampler images
const std::vector< std::pair<std::vector<std::string>, uint32_t> > allShaders_PostProcessPipeline =
{
	//passthrough: TEST
	{{"src/shaders/ppPassthrough.vert.spv",
	"src/shaders/ppPassthrough.frag.spv"},
	1},


};
//std::vector< std::tuple<std::string, int, glm::mat4> > defaultScene;
//const std::vector< std::tuple<std::string, int, glm::mat4> > defaultScene =
//{
//	//Model0 (path, isDynamic, modelmatrix)
//	{std::string("res/objects/rock/rock.obj"), 1,
//		glm::translate(glm::mat4(1.f),glm::vec3(0.f, 10.f, 0.f))},
//
//	//Model1
//	{std::string("res/objects/rock/rock.obj"), 1,
//		glm::translate(glm::mat4(1.f),glm::vec3(0.f, 3.f, 0.f))},
//};


const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifndef _DEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


