#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera(float fov, float aspectRatio, float nearPlane, float farPlane);

	void Update(float deltaTime);
	void ProcessKeyboard(float forward, float right, float up);
	void ProcessMouse(float xOffset, float yOffset, bool constrainPitch = true);

	glm::mat4 GetViewMatrix() const { return glm::lookAt(position, position + front, up); }
	glm::mat4 GetProjectionMatrix() const;

	void SetAspectRatio(float aspectRatio) { this->aspectRatio = aspectRatio; }

	glm::vec3 GetPosition() const { return position; }

private:
	void UpdateVectors();

	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	float yaw;
	float pitch;

	float fov;
	float aspectRatio;
	float nearPlane;
	float farPlane;

	float movementSpeed = 100.0f;
	float mouseSensitivity = 0.1f;
};

#endif // !CAMERA_H