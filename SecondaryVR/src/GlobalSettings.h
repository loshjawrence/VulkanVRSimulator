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


//const float hmdWidth = 1080;
//const float hmdHeight = 600;

const uint32_t hmdWidth = 1280;
const uint32_t hmdHeight = 800;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////// THESE ARE THE SHADERS A MESH CAN RUN BASED ON ITS MATERIAL FLAGS (DIFFUSE NORMAL HEIGHT SPEC)///////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const uint32_t HAS_NONE		= 1 << 0; 
const uint32_t HAS_DIFFUSE	= 1 << 1; 
const uint32_t HAS_NOR		= 1 << 2; 
const uint32_t HAS_SPEC		= 1 << 3;
const uint32_t HAS_HEIGHT	= 1 << 4;


//if you need to add a new one just add it here and it will make a new pipeline for it
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

///////////////////////////////////////////////////////////////////////
///////// THESE ARE THE PP STAGES THEY SHOULD PROCEED IN ORDER ////////
///////// EACH WILL PROCESS THE PREVIOUS STAGES OUTPUT ////////////////
///////////////////////////////////////////////////////////////////////
//could maked a tuple and add a flag to say which stage it sources its input image 
//for cases that aren't one after the other

//name of shaders and number of input sampler images
const bool useStencil = true;

//////////////////////////////////////SHADER PATHS,  num image inputs(should be vector of source stages)
const std::vector< std::tuple<std::vector<std::string>, uint32_t> > allShaders_PostProcessPipeline =
{
	////passthrough
	//{{"src/shaders/ppPassthrough.vert.spv",
	//"src/shaders/ppPassthrough.frag.spv"},
	//1}, //1 is num input sampler images to this pp stage

	////STENCIL HOLE FILL
	{{"src/shaders/ppPassthrough.vert.spv",
	"src/shaders/ppStencilHoleFill.frag.spv"},
	1}, 

	//////Barrel/Aberration all in FRAG 
	{{"src/shaders/ppPassthrough.vert.spv",
	"src/shaders/ppBarrelAbFragCommonUse.frag.spv"},
	1},

	//Barrel/Aberration PRECALC MESH
//	{{"src/shaders/ppBarrelAbMeshPreCalc.vert.spv",
//	"src/shaders/ppBarrelAbMeshPreCalc.frag.spv"},
//	1},

	//Barrel/Aberration SHADER MESH (no precalc, done in shaders)
	//{{"src/shaders/ppBarrelAbMesh2.vert.spv",
	//"src/shaders/ppBarrelAbMesh.frag.spv"},
	//1},
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


