#include "Camera.h"

Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane)
	: fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane)
{
}

void Camera::SetPosition(const glm::vec3& pos)
{
	position = pos;
}

void Camera::SetLookAt(const glm::vec3& target)
{
	lookAt = target;
}

void Camera::SetUp(const glm::vec3& upVec) 
{
	up = upVec;
}

glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(position, lookAt, up);
}

glm::mat4 Camera::GetProjectionMatrix() const
{
	return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}