#ifndef WORLDGEN_SYSTEM_H
#define WORLDGEN_SYSTEM_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <limits>

#include "MeshBuild.h"
#include "IWorldGenerator.h"
#include "DomainWarpedRidged.h"
#include "../rendering/MeshBatch.h"
#include "../rendering/Mesh.h"
#include "../rendering/VulkanDevice.h"

struct AABB
{
	glm::vec3 min{ 0 };
	glm::vec3 max{ 0 };
};

class WorldgenSystem
{
public:
	explicit WorldgenSystem(VulkanDevice& device, MeshBatch& batch)
		: device(device), batch(batch) {}

	// Settings
	WorldgenSettings settings;
	int viewRadius = 3; // Chunks in each direction from the camera (3 = 7x7 grid)

	// Life cycle
	void InitSingleChunk(uint32_t seed = 1337u);

	// Step 2 API
	void Update(const glm::vec3& camPos);
	void BuildVisibleSet(const glm::mat4& view, const glm::mat4& proj);

	// Acces
	const std::vector<Mesh*>& VisibleMeshes() const { return visibleMeshes; }

private:
	struct Chunk
	{
		WorldgenChunkKey key;
		std::unique_ptr<Mesh> mesh;
		AABB bounds;
	};

	VulkanDevice& device;
	MeshBatch& batch;
	std::unique_ptr<IWorldGenerator> generator;
	
	// Chunk storage keyed by (cx, cz)
	struct KeyHash
	{
		size_t operator()(const std::pair<int, int>& k) const noexcept {
			return (size_t)k.first * 73856093u ^ (size_t)k.second * 19349663u;
		}
	};
	std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, KeyHash> chunks;

	// Scratch list rebuilt each frame after culling
	std::vector<Mesh*> visibleMeshes;

	// Helpers
	float ChunkWorldSize() const
	{
		return float(settings.vertsPerSide - 1) * settings.cellSize;
	}
	void EnsureChunk(int cx, int cz);

	// Frustum extraction & test
	static void ExtractFrustumPlanes(const glm::mat4& vp, glm::vec4 outPlanes[6]);
	static bool AABBIntersectsFrustum(const AABB& b, const glm::vec4 planes[6]);

	//std::vector<std::unique_ptr<Mesh>> meshes;
};

#endif // !WORLDGEN_SYSTEM_H
