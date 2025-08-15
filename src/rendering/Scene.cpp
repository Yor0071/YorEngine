#include "Scene.h"
#include <iostream>

void Scene::AddInstance(const glm::mat4 transform, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, uint32_t meshIndex)
{
	instances.emplace_back(transform, std::move(mesh), std::move(material), meshIndex);
}

void Scene::UpdateMaterial(uint32_t index, std::shared_ptr<Material> newMaterial)
{
	if (index < instances.size())
	{
		instances[index].material = std::move(newMaterial);
	}
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