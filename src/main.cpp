#include "core/Window.h"
#include "rendering/VulkanRenderer.h"
#include <iostream>
#include <filesystem>

int main()
{
	try 
	{
		Window window(800, 600, "YorEngine");

		VulkanRenderer renderer;
		renderer.Init(window.GetWindow());

		while (!window.ShouldClose())
		{
			window.PollEvents();
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