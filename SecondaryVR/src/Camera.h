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
	float movementspeed = 5.f;
	float looksensitivity = 0.5f;
	float yaw = -90.f;//about y, looking down local -z
	float pitch = 0.f;//about x
	float fov = 45.f;
	float width = 1080;
	float height = 600;
	float near = 0.1f;
	float far = 100.f;
	glm::vec3 camPos = glm::vec3(0.f, 0.f, 3.f);
	glm::vec3 camFront;
	glm::vec3 camRight;
	glm::vec3 camUp;
	glm::mat4 view;
	glm::mat4 proj = glm::perspective(glm::radians(fov), width / height, near, far);

public:
	Camera();
	~Camera();
	void updateComponentVectorsAndViewAndUBO();
	void processKeyboardAndUpdateView(MovementDirection direction, float deltaTime);
	void processMouseAndUpdateView(float xoffset, float yoffset);
	void processScrollAndUpdateView(const float yoffset);

	void updateDimensionsAndUBO(const VkExtent2D& swapChainExtent);
	void updatePerspectiveProjection();
	void updateUBO();
};

