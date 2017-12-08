#include "Camera.h"
#include <iostream>

Camera::Camera() {
	vrScalings.resize(numQualitySettings);
	for (int i = 0; i < numQualitySettings; ++i) {
		vrScalings[i] = MAX_QUALITY - i*qualityStepping;
	}
	updatePerspectiveProjection();
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

	if (vrmode && !timewarp) {
		//right cam is left cam but local shift right by ipd
		const glm::vec3 rightCamPos = camPos + camRight*ipd;
		view[1] = glm::lookAt(rightCamPos, rightCamPos + camFront, camUp);
	} else if (vrmode && timewarp) {
		//when in time warp, both camera positions are locked and only rotation is updated
		//view[1] = glm::lookAt(rightCamPosTimeWarp, rightCamPosTimeWarp + camFront, camUp);
		const glm::vec3 rightCamPos = camPos + camRight*ipd;
		view[1] = glm::lookAt(rightCamPos, rightCamPos + camFront, camUp);
		//std::cout << "\nleft pos: " << camPos.x << " " << camPos.y << " " << camPos.z;
		//std::cout << "\nright pos: " << rightCamPosTimeWarp.x << " " << rightCamPosTimeWarp.y << " " << rightCamPosTimeWarp.z;
	}
}


void Camera::processKeyboardAndUpdateView(MovementDirection direction, float deltaTime) {
	//update camPos
	if (!timewarp) { //disable position updates
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
	}

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
	width = hmdWidth * scale * (vrmode ? vrScalings[qualityIndex] : 1.f);
	height = hmdHeight * (vrmode ? vrScalings[qualityIndex] : 1.f);

	//ensure that dims are even to avoid stencil issues
	width  = ((width  & 1) == 1) && !vrmode ? width  - 1 : width;
	height = ((height & 1) == 1)            ? height - 1 : height;

	//command buffers need the full render area
	renderTargetExtent.width = vrmode ? 2 * width : width;
	renderTargetExtent.height = height;

	//width = swapChainExtent.width * scale * (false ? vrScalings[qualityIndex] : 1.f);
	//height = swapChainExtent.height * (false ? vrScalings[qualityIndex] : 1.f);
	updatePerspectiveProjection();
}

void Camera::updatePerspectiveProjection() {
	proj = glm::perspective(glm::radians(fov), float(width) / height, near, far);
	proj[1][1] *= -1.f;
}

void Camera::updateVrModeAndCameras() {
	vrmode = !vrmode;
	if (!vrmode) { timewarp = false; }//force off in for non vr mode
	updateComponentVectorsAndViews(true);
}

//when in time warped state we only want to update camera rotation to avoid disocculusion(for now)
void Camera::updateTimeWarpState() {
	if (!timewarp) { 
		useStencil = false;
		timewarp = true;
		timewarpInitFlag = true; 
		rightCamPosTimeWarp = camPos + camRight*ipd;
	} else { 
		useStencil = true;
		timewarp = false;
		timewarpInitFlag = false;
	}
}

void Camera::timeWarpFinishInit(const uint32_t imageIndex) {
	timewarpTrippleBufferID = imageIndex;
	timewarpInitFlag = false;
	timeWarpInvVP[0] = glm::inverse(proj * view[0]);
	timeWarpInvVP[1] = glm::inverse(proj * view[1]);
}