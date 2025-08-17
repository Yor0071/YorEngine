#include "InputHandler.h"

InputHandler::InputHandler(GLFWwindow* window, Camera& camera)
	: window(window), camera(camera)
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
}

void InputHandler::Update(float deltaTime)
{
    float forward = 0.0f;
    float right = 0.0f;
	float up = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) right += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) right -= moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) up += moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) up -= moveSpeed;

    camera.ProcessKeyboard(forward * deltaTime, right * deltaTime, up * deltaTime);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse)
    {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - lastMouseX);
    float yoffset = static_cast<float>(lastMouseY - ypos);

    lastMouseX = xpos;
    lastMouseY = ypos;

	if (!cursorEnabled) camera.ProcessMouse(xoffset, yoffset);

	HandleToggleCursor();

    if (WasKeyJustPressed(GLFW_KEY_L) && onReloadShaders)
    {
        onReloadShaders();
    }
}

bool InputHandler::WasKeyJustPressed(int key)
{
    int state = glfwGetKey(window, key);

    bool wasDown = keyWasDown[key];
    keyWasDown[key] = (state == GLFW_PRESS);

    return state == GLFW_PRESS && !wasDown;
}

void InputHandler::EnableCursor()
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	cursorEnabled = true;
    firstMouse = true;
}

void InputHandler::DisableCursor()
{
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	cursorEnabled = false;
	firstMouse = true;
}

void InputHandler::HandleToggleCursor()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !cursorEnabled)
    {
        EnableCursor();
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && cursorEnabled)
    {
        DisableCursor();
    }
}