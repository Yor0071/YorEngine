#include "Camera.h"

Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
    : fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane),
    position(0.0f, 0.0f, 2.0f), worldUp(0.0f, 1.0f, 0.0f),
    yaw(-90.0f), pitch(0.0f)
{
    UpdateVectors();
}

void Camera::Update(float deltaTime)
{
	// Not used right now
}

void Camera::ProcessKeyboard(float forward, float right, float up)
{
	float velocity = movementSpeed;

	position += front * forward * velocity;
	position += this->right * right * velocity;
	position += this->up * up * velocity;
}

void Camera::ProcessMouse(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= mouseSensitivity;
	yoffset *= mouseSensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (constrainPitch)
	{
		if (pitch > 89.0f) pitch = 89.0f;
		if (pitch < -89.0f) pitch = -89.0f;
	}

	UpdateVectors();
}

glm::mat4 Camera::GetProjectionMatrix() const
{
	glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	proj[1][1] *= -1;
	return proj;
}

void Camera::UpdateVectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	this->front = glm::normalize(front);

	right = glm::normalize(glm::cross(this->front, worldUp));
	up = glm::normalize(glm::cross(right, this->front));
}