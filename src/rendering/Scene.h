#ifndef SCENE_H
#define SCENE_H

#define MODEL_PATH "../assets/models/cactus.fbx"

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "ModelLoader.h"
#include "ModelInstance.h"

class Scene
{
public:
	void Load(VulkanDevice& device);

	void AddModel(const std::string& filepath, VulkanDevice& device, const glm::mat4& transform);

	const std::vector<ModelInstance>& GetInstances() const { return instances; }

private:
	//std::vector<std::shared_ptr<Mesh>> loadedMeshes;
	std::vector<ModelInstance> instances;
};

#endif // !SCENE_H