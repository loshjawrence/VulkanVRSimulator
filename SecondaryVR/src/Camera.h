#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include "GlobalSettings.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class MovementDirection {
	NONE = -1, FORWARD = 0, BACKWARD = 1, LEFT = 2, RIGHT = 3, UP = 4, DOWN = 5,
};

class Camera {
public:
	glm::vec3 worldUp = glm::vec3(0.f, 1.f, 0.f);
	float ipd = 0.009f;//assets need to be made with scale in mind so this is just tuned to look reasonable
	float movementspeed = 10.f;
	float looksensitivity = 0.7f;
	float yaw = -90.f;//about y, looking down local -z
	float pitch = 0.f;//about x
	float fov = 45.f;
	
	//adaptive quality
	bool qualityEnabled = true;
	float targetFrameTime_ms = 2.f;
	//float MAX_QUALITY = 0.599999964f;//work around for odd scaling issue in holefill shader due to odd dims for x and y at certain intermediate scalings
	float MAX_QUALITY = 1.4;//work around for odd scaling issue in holefill shader due to odd dims for x and y at certain intermediate scalings

	//float virtualRenderTargetScaling = MAX_QUALITY;

	int qualityIndex = 0;
	int numQualitySettings = 10;
	float qualityStepping = 0.1f;
	std::vector<float> vrScalings;

	bool vrmode = false;
	//DK1 full
	//float width = 1280;
	//float height = 800;
	//DK1 virtual 1.4x(barrel will shrink to down)
	uint32_t width = hmdWidth;
	uint32_t height = hmdHeight;
	VkExtent2D renderTargetExtent = {width, height};
	//HALF modern
	//float width = 1080;
	//float height = 600;
	//FULL modern
	//float width = 2160;
	//float height = 1200;
	float near = 0.1f;
	float far = 1000.f;
	glm::vec3 camPos = glm::vec3(0.f, 3.f, 6.f);
	glm::vec3 camFront;
	glm::vec3 camRight;
	glm::vec3 camUp;
	glm::mat4 view[2];
	glm::mat4 proj = glm::perspective(glm::radians(fov), float(width) / height, near, far);


public:
	Camera();
	~Camera();
	void updateComponentVectorsAndViews(bool changingModes);
	void processKeyboardAndUpdateView(MovementDirection direction, float deltaTime);
	void processMouseAndUpdateView(float xoffset, float yoffset);
	void processScrollAndUpdateView(const float yoffset);

	void updateQualitySettings(const bool increase);
	void updateVrModeAndCameras();

	void updateDimensions(const VkExtent2D& swapChainExtent);
	void updatePerspectiveProjection();
};

