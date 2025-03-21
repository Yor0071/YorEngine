#include "Window.h"
#include <iostream>

Window::Window(int width, int height, const std::string& title)
	: width(width), height(height), title(title)
{
	InitWindow();
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::InitWindow()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
		{
			auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
			app->framebufferResized = true;
		});
}

bool Window::WasResized()
{
	return framebufferResized;
}

void Window::ResetResizeFlag()
{
	framebufferResized = false;
}

void Window::PollEvents()
{
	glfwPollEvents();
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(window);
}