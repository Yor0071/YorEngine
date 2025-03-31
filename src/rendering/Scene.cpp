#include "Scene.h"
#include <iostream>

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