#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera(float fov, float aspectRatio, float nearPlane, float farPlane);

	void SetPosition(const glm::vec3& pos);
	void SetLookAt(const glm::vec3& target);
	void SetUp(const glm::vec3& up);

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;

private:
	glm::vec3 position = { 0.0f, 0.0f, 2.0f };
	glm::vec3 lookAt = { 0.0f, 0.0f, 0.0f };
	glm::vec3 up = { 0.0f, 1.0f, 0.0f };

	float fov, aspectRatio, nearPlane, farPlane;
};

#endif // !CAMERA_H