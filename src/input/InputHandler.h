#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <GLFW/glfw3.h>
#include <unordered_map>

#include "../core/Camera.h"

class InputHandler
{
public:
	InputHandler(GLFWwindow* window, Camera& camera);

	void Update(float deltaTime);
	void EnableCursor();
	void DisableCursor();
	bool isCursorEnabled() const { return cursorEnabled; }
	bool WasKeyJustPressed(int key);

	std::function<void()> onReloadShaders;

	void SetOnReloadShaders(std::function<void()> callback)
	{
		onReloadShaders = std::move(callback);
	}

private:
	GLFWwindow* window;
	Camera& camera;

	double lastMouseX, lastMouseY;
	bool firstMouse = true;
	bool cursorEnabled = false;
	float moveSpeed = 0.01f;

	std::unordered_map<int, bool> keyWasDown;

	void HandleToggleCursor();
};

#endif // !INPUT_HANDLER_H
