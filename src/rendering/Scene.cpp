#include "Scene.h"
#include <iostream>

void Scene::Load(VulkanDevice& device)
{
    AddModel("../assets/models/cube.fbx", device, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f)));

    //AddModel("../assets/models/cactus.fbx", device, glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -4.0f)));
}

void Scene::AddModel(const std::string& filepath, VulkanDevice& device, const glm::mat4& transform)
{
    std::vector<ModelInstance> newInstances;

    if (ModelLoader::LoadModel(filepath, device, newInstances))
    {
        for (auto& instance : newInstances)
        {
            instance.transform = transform * instance.transform; // apply user-specified transform
            instances.push_back(std::move(instance));
        }

        std::cout << "[Scene] Added model with " << newInstances.size() << " instances." << std::endl;
    }
    else
    {
        std::cerr << "[Scene] Failed to load model: " << filepath << std::endl;
    }
}

void Scene::Clear()
{
    for (auto& instance : instances)
    {
        if (instance.mesh)
        {
            instance.mesh->Destroy();
        }
    }
    instances.clear();
}