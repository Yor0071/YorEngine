#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <GLFW/glfw3.h>
#include "../core/Camera.h"

class InputHandler
{
public:
	InputHandler(GLFWwindow* window, Camera& camera);

	void Update(float deltaTime);
	void EnableCursor();
	void DisableCursor();
	bool isCursorEnabled() const { return cursorEnabled; }
private:
	GLFWwindow* window;
	Camera& camera;

	double lastMouseX, lastMouseY;
	bool firstMouse = true;
	bool cursorEnabled = false;

	void HandleToggleCursor();
};

#endif // !INPUT_HANDLER_H
