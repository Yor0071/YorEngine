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

	static std::string GetMeshCachePath(const std::string& modelPath, unsigned int meshIndex);
	static std::string GetSceneCachePath(const std::string& modelPath);

	static bool LoadMeshFromCache(const std::string& path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	static void SaveMeshToCache(const std::string& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	static bool LoadSceneCache(const std::string& path, const std::vector<std::shared_ptr<Mesh>>& meshes, Scene& outScene);
	static void SaveSceneCache(const std::string& path, const Scene& scene, const std::vector<std::shared_ptr<Mesh>>& meshes);
	
	static std::unordered_map<unsigned int, std::shared_ptr<Mesh>> meshCache;
};

#endif // !MODEL_LOADER_H