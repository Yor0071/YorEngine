#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <vector>
#include <string>
#include <memory>

#include <glm/glm.hpp>

#include "../third_party/assimp/include/assimp/Importer.hpp"
#include "../third_party/assimp/include/assimp/scene.h"
#include "../third_party/assimp/include/assimp/postprocess.h"

#include "Vertex.h"
#include "Mesh.h"
#include "ModelInstance.h"
#include "VulkanDevice.h"

class ModelLoader
{
public:
    static bool LoadModel(const std::string& path, VulkanDevice& device, std::vector<ModelInstance>& outInstances);

private:
	static void ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform, VulkanDevice& device, std::vector<ModelInstance>& outInstances);

	static std::shared_ptr<Mesh> ProcessMesh(const aiMesh* mesh, VulkanDevice& device);

	static glm::mat4 ConvertMatrix(const aiMatrix4x4& matrix);

	static std::unordered_map<unsigned int, std::shared_ptr<Mesh>> meshCache;
};

#endif // !MODEL_LOADER_H