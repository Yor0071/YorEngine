#ifndef WORLDGEN_SYSTEM_H
#define WORLDGEN_SYSTEM_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
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

struct LODConfig
{
	int maxLOD = 4; // Max LOD level (0 = highest detail)
	float lodBase = 2.0f;
	float lodDistance = 80.0f; // Distance at which LOD changes
	int fullDetailRings = 1;
};

class WorldgenSystem
{
public:
	explicit WorldgenSystem(VulkanDevice& device, MeshBatch& batch)
		: device(device), batch(batch) {}

	// Settings
	WorldgenSettings settings;
	LODConfig lod;
	int viewRadius = 3; // Chunks in each direction from the camera (3 = 7x7 grid)

	// Life cycle
	void InitSingleChunk(uint32_t seed = 1337u);

	// Streaming API
	void Update(const glm::vec3& camPos);
	void BuildVisibleSet(const glm::mat4& view, const glm::mat4& proj);

	// Async life cycle
	void StartWorkers();
	void Shutdown();
	void PumpUploads(int budgetMeshesPerFrame = 2);

	// Access
	const std::vector<Mesh*>& VisibleMeshes() const { return visibleMeshes; }

	float SampleHeight(float x, float z) const;
	glm::vec3 SampleNormal(float x, float z) const;

private:
	struct Chunk
	{
		int cx = 0, cz = 0, lod = 0;
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
	
	// --- Async job system ---
	struct Job
	{
		int cx, cz, lod;
		float priority; // Lower is higher priority
	};

	struct Result
	{
		int cx, cz, lod;
		std::vector<Vertex> v;
		std::vector<uint32_t> i;
		AABB bounds;
	};

	std::vector<Job> jobQueue;
	std::unordered_set<long long> queuedKeys; // To avoid duplicates
	std::deque<Result> ready;

	std::thread worker;
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic<bool> stop{ false };

	void EnqueueJob(int cx, int cz, int lod, float priority);
	void WorkerLoop();

	// Build/rebuild GPU mesh immediately (used by InitSingleChunk)
	void EnsureChunkLOD_Immediate(int cx, int cz, int desiredLOD);

	// Frustum extraction & test
	static void ExtractFrustumPlanes(const glm::mat4& vp, glm::vec4 outPlanes[6]);
	static bool AABBIntersectsFrustum(const AABB& b, const glm::vec4 planes[6]);
};

#endif // !WORLDGEN_SYSTEM_H
