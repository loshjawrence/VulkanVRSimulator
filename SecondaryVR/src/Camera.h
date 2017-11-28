#pragma once
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // !GLFW_INCLUDE_VULKAN

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class MovementDirection {
	NONE = -1, FORWARD = 0, BACKWARD = 1, LEFT = 2, RIGHT = 3, UP = 4, DOWN = 5,
};

class Camera {
public:
	const glm::vec3 worldUp = glm::vec3(0.f, 1.f, 0.f);
	float ipd = 0.009f;//assets need to be made with scale in mind so this is just tuned to look reasonable
	float movementspeed = 10.f;
	float looksensitivity = 0.7f;
	float yaw = -90.f;//about y, looking down local -z
	float pitch = 0.f;//about x
	float fov = 45.f;
	//HALF
	float width = 1080;
	float height = 600;
	//FULL
	//float width = 2160;
	//float height = 1200;
	float near = 0.1f;
	float far = 1000.f;
	glm::vec3 camPos = glm::vec3(0.f, 3.f, 6.f);
	glm::vec3 camFront;
	glm::vec3 camRight;
	glm::vec3 camUp;
	glm::mat4 view[2];
	glm::mat4 proj = glm::perspective(glm::radians(fov), width / height, near, far);

	bool vrmode = false;

public:
	Camera();
	~Camera();
	void updateComponentVectorsAndViews(bool changingModes);
	void processKeyboardAndUpdateView(MovementDirection direction, float deltaTime);
	void processMouseAndUpdateView(float xoffset, float yoffset);
	void processScrollAndUpdateView(const float yoffset);

	void updateVrModeAndCameras();

	void updateDimensions(const VkExtent2D& swapChainExtent);
	void updatePerspectiveProjection();
};

