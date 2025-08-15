#ifndef MODEL_CACHE_MANAGER_H
#define MODEL_CACHE_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "Vertex.h"
#include "Mesh.h"
#include "Scene.h"

#include "../third_party/zstd/lib/zstd.h"
#include "../third_party/nlohmann/json.hpp"

class Scene;

class ModelCacheManager
{
public:
	static std::string GetMeshCachePath(const std::string& modelPath, unsigned int meshIndex);
	static std::string GetSceneCachePath(const std::string& modelPath);

	static bool LoadMeshFromCache(const std::string& path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	static void SaveMeshToCache(const std::string& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	static bool LoadSceneCache(const std::string& path, const std::vector<std::shared_ptr<Mesh>>& meshes, Scene& outScene);
	static void SaveSceneCache(const std::string& path, const Scene& scene, const std::vector<std::shared_ptr<Mesh>>& meshes);
	static std::unordered_map<std::string, std::shared_ptr<Material>> materialCache;
private:
};

#endif // !MODEL_CACHE_MANAGER_H
