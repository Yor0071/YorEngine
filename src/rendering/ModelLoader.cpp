#include "ModelLoader.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

bool ModelLoader::LoadModel(const std::string& path, VulkanDevice& device, MeshBatch& batch, Scene& outScene)
{
    outScene.Clear();
    std::vector<std::shared_ptr<Mesh>> loadedMeshes;

    std::string baseName = fs::path(path).stem().string();
    std::string cacheDir = "../assets/models/Cache/";
    std::string sceneCachePath = cacheDir + baseName + "_scene.json";

    bool allMeshesCached = true;

    // Try loading ALL meshes from cache
    unsigned int meshIndex = 0;
    while (true) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string meshCachePath = GetMeshCachePath(path, meshIndex);

        if (!fs::exists(meshCachePath))
            break;

        if (!LoadMeshFromCache(meshCachePath, vertices, indices)) {
            allMeshesCached = false;
            break;
        }

        // Upload to GPU
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

        loadedMeshes.push_back(meshPtr);
        ++meshIndex;
    }

    // If scene JSON is also cached and all meshes loaded, skip Assimp entirely
    if (allMeshesCached) {
        if (fs::exists(sceneCachePath)) {
            if (ModelLoader::LoadSceneCache(sceneCachePath, loadedMeshes, outScene)) {
                std::cout << "[ModelLoader] Loaded model and scene from cache.\n";
                return true;
            }
            else {
                std::cerr << "[ModelLoader] Scene cache exists but failed to load.\n";
            }
        }

        // Scene cache missing or invalid — fallback to Assimp for transform hierarchy
        std::cout << "[ModelLoader] Meshes cached, but scene cache missing or invalid. Rebuilding scene.\n";

        Assimp::Importer importer;
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

        if (!aiScene || !aiScene->HasMeshes()) {
            std::cerr << "[ModelLoader] Failed to reload scene with Assimp.\n";
            return false;
        }

        ProcessNode(aiScene->mRootNode, glm::mat4(1.0f), loadedMeshes, outScene);
        SaveSceneCache(sceneCachePath, outScene, loadedMeshes);
        std::cout << "[ModelLoader] Rebuilt and saved scene cache.\n";

        return true;
    }

	// Fallback to Assimp if not all meshes are cached
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
        std::cerr << "[ModelLoader] Failed to load: " << path << std::endl;
        return false;
    }

    std::cout << "[ModelLoader] Scene contains " << aiScene->mNumMeshes << " meshes.\n";

    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;
    size_t totalRawBytes = 0;
    size_t totalCompressedBytes = 0;

    outScene.Clear();

    for (unsigned int i = 0; i < aiScene->mNumMeshes; ++i) {
        const aiMesh* mesh = aiScene->mMeshes[i];
        if (!mesh->HasPositions()) continue;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        std::string cachePath = GetMeshCachePath(path, i);
        if (!LoadMeshFromCache(cachePath, vertices, indices)) {
            // Fill vertices
            for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                Vertex vertex{};
                vertex.pos = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
                vertex.color = mesh->HasNormals()
                    ? glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
                    : glm::vec3(1.0f);

                vertices.push_back(vertex);
            }

            // Fill indices
            for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                const aiFace& face = mesh->mFaces[f];
                for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                    indices.push_back(face.mIndices[j]);
                }
            }

            SaveMeshToCache(cachePath, vertices, indices);
            std::cout << "[ModelLoader] Cached: " << cachePath << "\n";
        }
        else {
            std::cout << "[ModelLoader] Loaded from cache: " << cachePath << "\n";
        }

        size_t rawSize = sizeof(Vertex) * vertices.size() + sizeof(uint32_t) * indices.size();
        totalRawBytes += rawSize;
        totalVertexCount += vertices.size();
        totalIndexCount += indices.size();

        std::cout << "[Mesh " << i << "] Vertices: " << vertices.size()
            << ", Indices: " << indices.size()
            << ", Raw: " << (rawSize / (1024.0 * 1024.0)) << " MB\n";

		if (vertices.empty() || indices.empty()) {
			std::cerr << "[ModelLoader] Mesh " << i << " has no vertices or indices.\n";
			continue;
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

        loadedMeshes.push_back(meshPtr);
    }

    std::cout << "\n[ModelLoader Summary]\n";
    std::cout << "Total Vertices: " << totalVertexCount << "\n";
    std::cout << "Total Indices: " << totalIndexCount << "\n";
    std::cout << "Total Raw Memory Size: " << (totalRawBytes / (1024.0 * 1024.0)) << " MB\n";

    ProcessNode(aiScene->mRootNode, glm::mat4(1.0f), loadedMeshes, outScene);

	//std::string sceneCachePath = GetSceneCachePath(path);
	SaveSceneCache(sceneCachePath, outScene, loadedMeshes);

    return true;
}

void ModelLoader::ProcessNode(aiNode* node, const glm::mat4& parentTransform, const std::vector<std::shared_ptr<Mesh>>& loadedMeshes, Scene& outScene)
{
    glm::mat4 transform = parentTransform * ConvertMatrix(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex >= loadedMeshes.size()) continue;

        outScene.AddInstance(transform, loadedMeshes[meshIndex], meshIndex);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], transform, loadedMeshes, outScene);
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

std::string ModelLoader::GetMeshCachePath(const std::string& modelPath, unsigned int meshIndex)
{
    std::string baseName = fs::path(modelPath).stem().string(); // e.g., "Sponza"
    std::string cacheDir = "../assets/models/Cache/";

    if (!fs::exists(cacheDir)) {
        fs::create_directories(cacheDir);
    }

    return cacheDir + baseName + "_mesh_" + std::to_string(meshIndex) + ".bin";
}

bool ModelLoader::LoadMeshFromCache(const std::string& path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Failed to open mesh cache file: " << path << std::endl;
        return false;
    }

    size_t compressedSize = 0;
    size_t originalSize = 0;
    in.read(reinterpret_cast<char*>(&compressedSize), sizeof(size_t));
    in.read(reinterpret_cast<char*>(&originalSize), sizeof(size_t));

    std::vector<uint8_t> compressed(compressedSize);
    std::vector<uint8_t> rawData(originalSize);

    in.read(reinterpret_cast<char*>(compressed.data()), compressedSize);

    size_t result = ZSTD_decompress(rawData.data(), originalSize, compressed.data(), compressedSize);
    if (ZSTD_isError(result)) {
        std::cerr << "Decompression failed: " << ZSTD_getErrorName(result) << std::endl;
        return false;
    }

    const uint8_t* ptr = rawData.data();
    uint32_t vertexCount = 0, indexCount = 0;

    memcpy(&vertexCount, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(&indexCount, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

    vertices.resize(vertexCount);
    indices.resize(indexCount);

    memcpy(vertices.data(), ptr, sizeof(Vertex) * vertexCount); ptr += sizeof(Vertex) * vertexCount;
    memcpy(indices.data(), ptr, sizeof(uint32_t) * indexCount);

    return true;
}

void ModelLoader::SaveMeshToCache(const std::string& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
    uint32_t indexCount = static_cast<uint32_t>(indices.size());

    std::vector<uint8_t> rawData;
    rawData.resize(sizeof(uint32_t) * 2 + sizeof(Vertex) * vertexCount + sizeof(uint32_t) * indexCount);

    uint8_t* ptr = rawData.data();
    memcpy(ptr, &vertexCount, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &indexCount, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, vertices.data(), sizeof(Vertex) * vertexCount); ptr += sizeof(Vertex) * vertexCount;
    memcpy(ptr, indices.data(), sizeof(uint32_t) * indexCount);

    size_t maxCompressedSize = ZSTD_compressBound(rawData.size());
    std::vector<uint8_t> compressed(maxCompressedSize);

    size_t compressedSize = ZSTD_compress(compressed.data(), maxCompressedSize, rawData.data(), rawData.size(), 1);
    if (ZSTD_isError(compressedSize)) {
        throw std::runtime_error("Compression failed: " + std::string(ZSTD_getErrorName(compressedSize)));
    }

    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&compressedSize), sizeof(size_t));
	size_t rawSize = rawData.size();
    out.write(reinterpret_cast<const char*>(&rawSize), sizeof(size_t)); // original size
    out.write(reinterpret_cast<const char*>(compressed.data()), compressedSize);
}

std::string ModelLoader::GetSceneCachePath(const std::string& modelPath)
{
    std::string baseName = fs::path(modelPath).stem().string();
    std::string cacheDir = "../assets/models/Cache/";

    if (!fs::exists(cacheDir)) {
        fs::create_directories(cacheDir);
    }

    return cacheDir + baseName + "_scene.json";
}

bool ModelLoader::LoadSceneCache(const std::string& path, const std::vector<std::shared_ptr<Mesh>>& meshes, Scene& outScene)
{
    std::ifstream in(path);
    if (!in.is_open()) return false;

    nlohmann::json j;
    in >> j;

    for (auto& entry : j) {
        uint32_t meshIndex = entry["meshIndex"];
        if (meshIndex >= meshes.size()) continue;

        glm::mat4 transform{};
        auto& mat = entry["transform"];
        for (int i = 0; i < 16; ++i) {
            transform[i / 4][i % 4] = mat[i];
        }

        outScene.AddInstance(transform, meshes[meshIndex], meshIndex);
    }

    return true;
}

void ModelLoader::SaveSceneCache(const std::string& path, const Scene& scene, const std::vector<std::shared_ptr<Mesh>>& meshes)
{
    nlohmann::json j;

    for (const auto& inst : scene.GetInstances()) {
        nlohmann::json entry;
        entry["meshIndex"] = inst.meshIndex;
        std::vector<float> mat(16);
        const glm::mat4& t = inst.transform;
        for (int i = 0; i < 16; ++i) {
            mat[i] = t[i / 4][i % 4];
        }
        entry["transform"] = mat;
        j.push_back(entry);
    }

    std::ofstream out(path);
    out << j.dump(2);
}
