#include "Camera.h"

Camera::Camera() {
	updateComponentVectorsAndViewAndUBO();
}

Camera::~Camera() {
}

void Camera::updateComponentVectorsAndViewAndUBO() {
	camFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	camFront.y = sin(glm::radians(pitch));
	camFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	camFront = glm::normalize(camFront);
	// Also re-calculate the Right and Up vector
	camRight = glm::normalize(glm::cross(camFront, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	camUp = glm::normalize(glm::cross(camRight, camFront));
	view = glm::lookAt(camPos, camPos + camFront, camUp);

	updateUBO();
}


void Camera::processKeyboardAndUpdateView(MovementDirection direction, float deltaTime) {
	//update camPos
	float velocity = movementspeed * deltaTime;
	if (direction == MovementDirection::FORWARD)
		camPos += camFront * velocity;
	if (direction == MovementDirection::BACKWARD)
		camPos -= camFront * velocity;
	if (direction == MovementDirection::LEFT)
		camPos -= camRight * velocity;
	if (direction == MovementDirection::RIGHT)
		camPos += camRight * velocity;
	if (direction == MovementDirection::UP)
		camPos += camUp * velocity;
	if (direction == MovementDirection::DOWN)
		camPos -= camUp * velocity;

	updateComponentVectorsAndViewAndUBO();
}

void Camera::processMouseAndUpdateView(float xoffset, float yoffset) {
	//update camFront
	xoffset *= looksensitivity;
	yoffset *= looksensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	updateComponentVectorsAndViewAndUBO();
}

void Camera::processScrollAndUpdateView(const float yoffset) {
	fov -= yoffset;
	glm::clamp(fov, 1.f, 45.f);
	updateComponentVectorsAndViewAndUBO();
}

void Camera::updateDimensionsAndUBO(const VkExtent2D& swapChainExtent) {
	width = swapChainExtent.width;
	height = swapChainExtent.height;
	updatePerspectiveProjection();
	updateUBO();
}

void Camera::updatePerspectiveProjection() {
	proj = glm::perspective(glm::radians(fov), width / height, near, far);
}

void Camera::updateUBO() {

}
