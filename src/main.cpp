#include "core/Window.h"
#include "rendering/VulkanRenderer.h"
#include <iostream>

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
		}

		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
}