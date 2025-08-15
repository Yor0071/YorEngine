#include "core/Window.h"
#include "rendering/VulkanRenderer.h"
#include "rendering/ModelLoader.h"
#include "rendering/Vertex.h"
#include "input/InputHandler.h"
#include "ecs/TerrainComponent.h"
#include "utils/PerlinNoise.h"
#include "rendering/Mesh.h"
#include "rendering/MeshBatch.h"
#include "rendering/VulkanDevice.h"
#include <iostream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

std::unique_ptr<Mesh> terrainMesh;

int main()
{
	try 
	{
		Window window(800, 600, "YorEngine");

		VulkanRenderer renderer;
		renderer.Init(window.GetWindow());

		renderer.InitTerrain();

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
			// ../assets/models/cactus.fbx

			if (!ModelLoaded && glfwGetKey(window.GetWindow(), GLFW_KEY_M) == GLFW_PRESS)
			{
				const std::string modelPath = "../assets/models/Main.1_Sponza/NewSponza_Main_Yup_003.fbx";
				renderer.LoadModelAsync(modelPath);
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