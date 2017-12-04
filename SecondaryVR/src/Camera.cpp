#include "Camera.h"

Camera::Camera() {
	vrScalings.resize(numQualitySettings);
	for (int i = 0; i < numQualitySettings; ++i) {
		vrScalings[i] = MAX_QUALITY - i*qualityStepping;
	}
	updateComponentVectorsAndViews(false);
}

Camera::~Camera() {
}

void Camera::updateComponentVectorsAndViews(bool changingModes) {
	if (changingModes) {
		const float shift = vrmode ? -ipd / 2.f : ipd / 2.f;//if vrmode: local left shift, else local right shift
		camPos += camRight*shift;
	}
	camFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	camFront.y = sin(glm::radians(pitch));
	camFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	camFront = glm::normalize(camFront);
	// Also re-calculate the Right and Up vector
	camRight = glm::normalize(glm::cross(camFront, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	camUp = glm::normalize(glm::cross(camRight, camFront));
	view[0] = glm::lookAt(camPos, camPos + camFront, camUp);

	if (vrmode) {
		//right cam is left cam but local shift right by ipd
		const glm::vec3 rightCamPos = camPos + camRight*ipd;
		view[1] = glm::lookAt(rightCamPos, rightCamPos + camFront, camUp);
	} 
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

	updateComponentVectorsAndViews(false);
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

	updateComponentVectorsAndViews(false);
}

void Camera::processScrollAndUpdateView(const float yoffset) {
	fov -= yoffset;
	glm::clamp(fov, 1.f, 45.f);
	updateComponentVectorsAndViews(false);
}

void Camera::updateQualitySettings(const bool increase) {
	qualityIndex += increase ? -1 : 1;//yes, 0 is index of highest
	qualityIndex = glm::clamp(qualityIndex, 0, numQualitySettings - 1);

}

void Camera::updateDimensions(const VkExtent2D& swapChainExtent) {
	const float scale = vrmode ? 0.5f : 1.f;
	width = swapChainExtent.width * scale * (vrmode ? vrScalings[qualityIndex] : 1.f);
	height = swapChainExtent.height * (vrmode ? vrScalings[qualityIndex] : 1.f);

	//ensure that dims are even to avoid stencil issues
	width = ((uint32_t)width & 1) == 1 ? width - 1 : width;
	height = ((uint32_t)height & 1) == 1 ? height - 1 : height;

	//width = swapChainExtent.width * scale * (false ? vrScalings[qualityIndex] : 1.f);
	//height = swapChainExtent.height * (false ? vrScalings[qualityIndex] : 1.f);
	updatePerspectiveProjection();
}

void Camera::updatePerspectiveProjection() {
	proj = glm::perspective(glm::radians(fov), width / height, near, far);
}

void Camera::updateVrModeAndCameras() {
	vrmode = !vrmode;
	updateComponentVectorsAndViews(true);
}