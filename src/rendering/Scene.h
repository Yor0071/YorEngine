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
#include "Material.h"

class Scene
{
public:
	void AddInstance(const glm::mat4 transform, std::shared_ptr<Mesh> mesh,std::shared_ptr<Material> material , uint32_t meshIndex);
	const std::vector<ModelInstance>& GetInstances() const { return instances; }
	void UpdateMaterial(uint32_t index, std::shared_ptr<Material> newMaterial);

	void Upload(VulkanDevice& device);
	const MeshBatch& GetMeshBatch() const { return *meshBatch; }
	MeshBatch& GetMeshBatch() { return *meshBatch; }
	VulkanDevice& GetDevice() { return *device; }
	
	void SetMeshBatch(MeshBatch* batch) { meshBatch = batch; }
	void SetDevice(VulkanDevice* device) { this->device = device; }
	void SetMaterialPool(VkDescriptorPool pool) { materialPool = pool; }
	VkDescriptorPool GetMaterialPool() const { return materialPool; }

	void Clear();
private:
	std::vector<ModelInstance> instances;
	MeshBatch* meshBatch = nullptr;
	VulkanDevice* device = nullptr;
	VkDescriptorPool materialPool = VK_NULL_HANDLE;
};

#endif // !SCENE_H