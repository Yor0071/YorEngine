#include "Scene.h"
#include <iostream>

void Scene::Load(VulkanDevice& device, MeshBatch& batch)
{
	this->device = &device;
	this->meshBatch = &batch;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	if (!ModelLoader::LoadModel(MODEL_PATH, device, batch, *this))
	{
		std::cerr << "Failed to load model" << std::endl;
		return;
	}
}

void Scene::AddInstance(const glm::mat4 transform, std::shared_ptr<Mesh> mesh, uint32_t meshIndex)
{
	instances.push_back({ transform, mesh, meshIndex });
}

void Scene::Upload(VulkanDevice& device)
{
	if (meshBatch)
	{
		meshBatch->UploadToGPU(device);
	}
}

void Scene::Clear()
{
	if (device)
	{
		vkDeviceWaitIdle(device->GetLogicalDevice());
	}

	if (meshBatch)
	{
		meshBatch->Destroy(device->GetLogicalDevice());
	}
	instances.clear();
}