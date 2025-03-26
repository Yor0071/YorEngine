#include "core/Window.h"
#include "rendering/VulkanRenderer.h"
#include "input/InputHandler.h"
#include <iostream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

int main()
{
	try 
	{
		Window window(800, 600, "YorEngine");

		VulkanRenderer renderer;
		renderer.Init(window.GetWindow());

		float lastTime = glfwGetTime();

		bool ModelLoaded = false;

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

			// ../assets/models/Main.1_Sponza/NewSponza_Main_Yup_003.fbx

			if (!ModelLoaded && glfwGetKey(window.GetWindow(), GLFW_KEY_M) == GLFW_PRESS)
			{
				renderer.GetScene().AddModel(
					"../assets/models/cactus.fbx",
					*renderer.GetDevice(),
					glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f))
				);
				ModelLoaded = true;
			}

			if (glfwGetKey(window.GetWindow(), GLFW_KEY_M) == GLFW_RELEASE)
			{
				ModelLoaded = false; // debounce
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