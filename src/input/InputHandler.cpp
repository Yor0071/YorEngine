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

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) right += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) right -= 1.0f;

    camera.ProcessKeyboard(forward * deltaTime, right * deltaTime, 0.0f);

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse)
    {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - lastMouseX);
    float yoffset = static_cast<float>(ypos - lastMouseY);

    lastMouseX = xpos;
    lastMouseY = ypos;

    camera.ProcessMouse(xoffset, yoffset);
}