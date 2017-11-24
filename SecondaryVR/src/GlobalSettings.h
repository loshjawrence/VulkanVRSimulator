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


const std::string VERT_SHADER_PATH = "src/shaders/vert.spv";
const std::string FRAG_SHADER_PATH = "src/shaders/frag.spv";

const std::vector<std::string> forwardRender_shaders = { VERT_SHADER_PATH, FRAG_SHADER_PATH };
const std::vector< std::vector<std::string> > allShaders_ForwardPipeline =
{
	//subpass0 shaders
	forwardRender_shaders,

	//subpass1 shaders
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


