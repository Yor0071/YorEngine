#include "ModelCacheManager.h"

namespace fs = std::filesystem;

std::string ModelCacheManager::GetMeshCachePath(const std::string& modelPath, unsigned int meshIndex)
{
    std::string baseName = fs::path(modelPath).stem().string();
    std::string cacheDir = "../assets/models/Cache/";

    if (!fs::exists(cacheDir)) {
        fs::create_directories(cacheDir);
    }

    return cacheDir + baseName + "_mesh_" + std::to_string(meshIndex) + ".bin";
}

std::string ModelCacheManager::GetSceneCachePath(const std::string& modelPath)
{
	std::string baseName = fs::path(modelPath).stem().string();
	std::string cacheDir = "../assets/models/Cache/";

	if (!fs::exists(cacheDir)) {
		fs::create_directories(cacheDir);
	}

	return cacheDir + baseName + "_scene.json";
}

bool ModelCacheManager::LoadMeshFromCache(const std::string& path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
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

void ModelCacheManager::SaveMeshToCache(const std::string& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
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
    out.write(reinterpret_cast<const char*>(&rawSize), sizeof(size_t));
    out.write(reinterpret_cast<const char*>(compressed.data()), compressedSize);
}

bool ModelCacheManager::LoadSceneCache(const std::string& path, const std::vector<std::shared_ptr<Mesh>>& meshes, Scene& outScene)
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

void ModelCacheManager::SaveSceneCache(const std::string& path, const Scene& scene, const std::vector<std::shared_ptr<Mesh>>& meshes)
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