#include "Scene.h"
#include <iostream>

void Scene::Load(VulkanDevice& device)
{
	AddModel("../assets/models/cube.fbx", device, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f)));

	//AddModel("../assets/models/cactus.fbx", device, glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -4.0f)));
}

const std::vector<ModelInstance>& Scene::GetInstances() const
{
	return instances;
}

void Scene::AddModel(const std::string& filepath, VulkanDevice& device, const glm::mat4& transform)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	if (ModelLoader::LoadModel(filepath, vertices, indices))
	{
		auto mesh = std::make_shared<Mesh>(device, vertices, indices);
		loadedMeshes.push_back(mesh);

		ModelInstance instance;
		instance.mesh = mesh;
		instance.transform = transform;
		instances.push_back(instance);

		std::cout << "[Scene] Added model: " << filepath << std::endl;
	}
	else
	{
		std::cerr << "[Scene] Failed to load model: " << filepath << std::endl;
	}
}