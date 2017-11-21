#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include <string>
#include <vector>
#include <string>

const int WIDTH = 800;
const int HEIGHT = 600;

const std::string VERT_SHADER_PATH = "src/shaders/vert.spv";
const std::string FRAG_SHADER_PATH = "src/shaders/frag.spv";

const std::vector<std::string> forwardRender_shaders = { VERT_SHADER_PATH, FRAG_SHADER_PATH };
const std::vector< std::vector<std::string> > allShaders_DefaultPipeline =
{
	//subpass0 shaders
	forwardRender_shaders,
};

//const std::string MODEL_PATH = "res/objects/chalet.obj";
//const std::string TEXTURE_PATH = "res/textures/chalet.jpg";
const std::string MODEL_PATH = "res/objects/rock/rock.obj";
const std::string TEXTURE_PATH = "res/objects/rock/rock.png";

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif