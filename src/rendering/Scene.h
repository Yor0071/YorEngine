#ifndef SCENE_H
#define SCENE_H

#define MODEL_PATH "../assets/models/cube.fbx"

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Meshbatch.h"
#include "ModelLoader.h"
#include "ModelInstance.h"

class Scene
{
public:
	void Load(VulkanDevice& device, MeshBatch& batch);
	void AddInstance(const glm::mat4 transform, std::shared_ptr<Mesh> mesh, uint32_t meshIndex);

	const std::vector<ModelInstance>& GetInstances() const { return instances; }

	void Upload(VulkanDevice& device);
	const MeshBatch& GetMeshBatch() const { return *meshBatch; }
	MeshBatch& GetMeshBatch() { return *meshBatch; }

	void Clear();
	void SetDevice(VulkanDevice* device) { this->device = device; }
private:
	std::vector<ModelInstance> instances;
	MeshBatch* meshBatch = nullptr;
	VulkanDevice* device = nullptr;
};

#endif // !SCENE_H