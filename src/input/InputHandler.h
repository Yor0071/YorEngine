#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <GLFW/glfw3.h>
#include "../core/Camera.h"

class InputHandler
{
public:
	InputHandler(GLFWwindow* window, Camera& camera);

	void Update(float deltaTime);

private:
	GLFWwindow* window;
	Camera& camera;

	double lastMouseX, lastMouseY;
	bool firstMouse = true;
};

#endif // !INPUT_HANDLER_H
