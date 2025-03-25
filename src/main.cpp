#include "core/Window.h"
#include "rendering/VulkanRenderer.h"
#include "input/InputHandler.h"
#include <iostream>
#include <filesystem>

int main()
{
	try 
	{
		Window window(800, 600, "YorEngine");

		VulkanRenderer renderer;
		renderer.Init(window.GetWindow());

		float lastTime = glfwGetTime();

		while (!window.ShouldClose())
		{
			window.PollEvents();

			float currentTime = glfwGetTime();
			float deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			renderer.Update(deltaTime);

			if (window.WasResized())
			{
				renderer.ReCreateSwapChain(window.GetWindow());
				window.ResetResizeFlag();
			}

			renderer.DrawFrame();
		}

		renderer.Cleanup();

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
}