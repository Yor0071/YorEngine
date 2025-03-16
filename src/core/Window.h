#ifndef WINDOW_H
#define WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class Window
{
public:
	Window(int width, int height, const std::string& title);
	~Window();

	void PollEvents();
	bool ShouldClose() const;

	GLFWwindow* GetWindow() const { return window; }

private:
	void InitWindow();

	int width, height;
	std::string title;
	GLFWwindow* window;
};

#endif // !WINDOW_H


