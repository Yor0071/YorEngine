#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <filesystem>
#include <glm/glm.hpp>

#include "../third_party/assimp/include/assimp/Importer.hpp"
#include "../third_party/assimp/include/assimp/scene.h"
#include "../third_party/assimp/include/assimp/postprocess.h"
#include "../third_party/zstd/lib/zstd.h"
#include "../third_party/zstd/lib/zstd_errors.h"
#include "../third_party/nlohmann/json.hpp"

#include "Vertex.h"
#include "Mesh.h"
#include "ModelInstance.h"
#include "VulkanDevice.h"
#include "Scene.h"
#include "ModelCacheManager.h"

class Scene;

class ModelLoader
{
public:
	struct CachedInstance
	{
		uint32_t meshIndex;
		glm::mat4 transform;
	};

	static bool LoadModel(const std::string& path, VulkanDevice& device, MeshBatch& batch, Scene& outScene);


private:
	static void ProcessNode(aiNode* node, const glm::mat4& parentTransform, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes, Scene& outScene);

	static glm::mat4 ConvertMatrix(const aiMatrix4x4& matrix);

	static bool TryLoadCachedMeshes(const std::string& path, VulkanDevice& device, MeshBatch& batch, std::vector<std::shared_ptr<Mesh>>& outMeshes);
	static void LoadWithAssimp(const std::string& path, VulkanDevice& device, MeshBatch& batch, Scene& outScene, std::vector<std::shared_ptr<Mesh>>& outMeshes);
	
	static std::unordered_map<unsigned int, std::shared_ptr<Mesh>> meshCache;
};

#endif // !MODEL_LOADER_H