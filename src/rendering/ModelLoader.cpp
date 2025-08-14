#include "ModelLoader.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

bool ModelLoader::LoadModel(const std::string& path, VulkanDevice& device, MeshBatch& batch, Scene& outScene, VkDescriptorPool materialPool)
{
    outScene.Clear();
	outScene.SetDevice(&device);
	outScene.SetMaterialPool(materialPool);
    std::vector<std::shared_ptr<Mesh>> loadedMeshes;

    std::string sceneCachePath = ModelCacheManager::GetSceneCachePath(path);

    if (TryLoadCachedMeshes(path, device, batch, loadedMeshes))
    {
        if (ModelCacheManager::LoadSceneCache(sceneCachePath, loadedMeshes, outScene))
        {
            std::cout << "[ModelLoader] Loaded model and scene from cache.\n";
            return true;
        }
        else
        {
            std::cerr << "[ModelLoader] Scene cache missing or invalid. Reloading full model.\n";
        }
    }

    LoadWithAssimp(path, device, batch, outScene, loadedMeshes, materialPool);
    ModelCacheManager::SaveSceneCache(sceneCachePath, outScene, loadedMeshes);
    return true;
}

bool ModelLoader::TryLoadCachedMeshes(const std::string& path, VulkanDevice& device, MeshBatch& batch, std::vector<std::shared_ptr<Mesh>>& outMeshes)
{
    unsigned int meshIndex = 0;

    while (true) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string meshCachePath = ModelCacheManager::GetMeshCachePath(path, meshIndex);

        if (!fs::exists(meshCachePath)) break;

        if (!ModelCacheManager::LoadMeshFromCache(meshCachePath, vertices, indices)) {
            std::cerr << "[ModelLoader] Failed to load mesh cache: " << meshCachePath << "\n";
            return false;
        }

        MeshBatch::MeshRange range{};
        batch.UploadMeshToGPU(device, vertices, indices, range);
        const auto& gpuMesh = batch.GetLastUploadedMesh();

        auto meshPtr = std::make_shared<Mesh>(
            gpuMesh.vertexBuffer,
            gpuMesh.vertexMemory,
            gpuMesh.indexBuffer,
            gpuMesh.indexMemory,
            range
        );

        outMeshes.push_back(meshPtr);
        ++meshIndex;
    }

    return !outMeshes.empty();
}

void ModelLoader::LoadWithAssimp(const std::string& path, VulkanDevice& device, MeshBatch& batch, Scene& outScene, std::vector<std::shared_ptr<Mesh>>& outMeshes, VkDescriptorPool materialPool)
{
	Assimp::Importer importer;
	auto start = std::chrono::high_resolution_clock::now();

	const aiScene* aiScene = importer.ReadFile(path,
		aiProcess_Triangulate |
		aiProcess_GenNormals |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ImproveCacheLocality |
		aiProcess_OptimizeMeshes |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_PreTransformVertices |
		aiProcess_FlipUVs |
		aiProcess_ConvertToLeftHanded
	);

	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "[Assimp] Load time: " << std::chrono::duration<double>(end - start).count() << "s\n";

	if (!aiScene || !aiScene->HasMeshes()) {
		throw std::runtime_error("[ModelLoader] Failed to load model: " + path);
	}

	std::cout << "[ModelLoader] Scene contains " << aiScene->mNumMeshes << " meshes.\n";

	for (unsigned int i = 0; i < aiScene->mNumMeshes; ++i) {
		const aiMesh* mesh = aiScene->mMeshes[i];
		if (!mesh->HasPositions()) continue;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		std::string cachePath = ModelCacheManager::GetMeshCachePath(path, i);
		if (!ModelCacheManager::LoadMeshFromCache(cachePath, vertices, indices)) {
			for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
				Vertex vertex{};
				vertex.pos = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };

				vertex.color = mesh->HasNormals()
					? glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
					: glm::vec3(1.0f);

				vertex.uv = mesh->HasTextureCoords(0)
					? glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
					: glm::vec2(0.0f);

				vertices.push_back(vertex);
			}

			for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
				const aiFace& face = mesh->mFaces[f];
				for (unsigned int j = 0; j < face.mNumIndices; ++j) {
					indices.push_back(face.mIndices[j]);
				}
			}

			ModelCacheManager::SaveMeshToCache(cachePath, vertices, indices);
			std::cout << "[ModelLoader] Cached: " << cachePath << "\n";
		}
		else {
			std::cout << "[ModelLoader] Loaded from cache: " << cachePath << "\n";
		}

		MeshBatch::MeshRange range{};
		batch.UploadMeshToGPU(device, vertices, indices, range);
		const auto& gpuMesh = batch.GetLastUploadedMesh();

		auto meshPtr = std::make_shared<Mesh>(
			gpuMesh.vertexBuffer,
			gpuMesh.vertexMemory,
			gpuMesh.indexBuffer,
			gpuMesh.indexMemory,
			range
		);

		outMeshes.push_back(meshPtr);
	}

	ProcessNode(aiScene->mRootNode, glm::mat4(1.0f), outMeshes, outScene, device, aiScene, materialPool);
}

void ModelLoader::ProcessNode(aiNode* node, const glm::mat4& parentTransform, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes, Scene& outScene, VulkanDevice& device, const aiScene* aiScene, VkDescriptorPool materialPool)
{
	glm::mat4 transform = parentTransform * ConvertMatrix(node->mTransformation);

	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
		uint32_t meshIndex = node->mMeshes[i];
		if (meshIndex >= loadedMeshes.size()) continue;

		std::string texPathStr = "../assets/models/Main.1_Sponza/textures/default.png";
		std::shared_ptr<Material> material;

		if (aiScene->HasMaterials()) {
			const aiMesh* mesh = aiScene->mMeshes[meshIndex];
			if (mesh->mMaterialIndex < aiScene->mNumMaterials) {
				aiMaterial* aiMat = aiScene->mMaterials[mesh->mMaterialIndex];
				aiString texPath;
				if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
					fs::path fullPath = fs::path(texPath.C_Str());
					if (!fullPath.is_absolute()) {
						fullPath = fs::path(device.GetAssetBasePath()) / fullPath;
					}
					texPathStr = fullPath.string();
				}
			}

			// Use material cache to avoid reloading textures
			auto it = ModelCacheManager::materialCache.find(texPathStr);
			if (it != ModelCacheManager::materialCache.end()) {
				material = it->second;
			}
			else {
				material = CreateSafeMaterial(device, texPathStr, materialPool);
				ModelCacheManager::materialCache[texPathStr] = material;
			}
		}

		// Fallback if material still not assigned
		if (!material) {
			material = CreateSafeMaterial(device, "../assets/models/Main.1_Sponza/textures/default.png", materialPool);
		}

		outScene.AddInstance(transform, loadedMeshes[meshIndex], material, meshIndex);
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		ProcessNode(node->mChildren[i], transform, loadedMeshes, outScene, device, aiScene, materialPool);
	}
}

std::shared_ptr<Material> ModelLoader::CreateSafeMaterial(VulkanDevice& device, const std::string& path, VkDescriptorPool materialPool)
{
	try {
		return std::make_shared<Material>(device, path, materialPool);
	}
	catch (...) {
		std::cerr << "[Material] Failed to load: " << path << ", using fallback.\n";
		return std::make_shared<Material>(device, "../assets/models/Main.1_Sponza/textures/default.png", materialPool);
	}
}

glm::mat4 ModelLoader::ConvertMatrix(const aiMatrix4x4& m)
{
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}