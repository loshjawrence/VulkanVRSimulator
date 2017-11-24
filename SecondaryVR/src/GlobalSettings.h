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


const std::vector< std::vector<std::string> > allShaders_ForwardPipeline =
{
	//No Tex shaders
	{"src/shaders/forward.vert.spv", 
	 "src/shaders/forwardNoTex.frag.spv"},

	//forwardDiffuse shaders
	{"src/shaders/forward.vert.spv", 
	 "src/shaders/forwardDiffuse.frag.spv"},

	//forwardNor shaders
	{"src/shaders/forward.vert.spv", 
	 "src/shaders/forwardNor.frag.spv"},

	//forwardSpec shaders
	{"src/shaders/forward.vert.spv", 
	 "src/shaders/forwardSpec.frag.spv"},

	//forwardHeight shaders
	{"src/shaders/forward.vert.spv", 
	 "src/shaders/forwardHeight.frag.spv"},

	//forwardAll shaders
	{"src/shaders/forward.vert.spv", 
	 "src/shaders/forwardAll.frag.spv"},
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


