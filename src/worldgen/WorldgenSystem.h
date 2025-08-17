#ifndef WORLDGEN_SYSTEM_H
#define WORLDGEN_SYSTEM_H

#include <memory>
#include <vector>

#include "MeshBuild.h"
#include "IWorldGenerator.h"
#include "DomainWarpedRidged.h"
#include "../rendering/MeshBatch.h"
#include "../rendering/Mesh.h"
#include "../rendering/VulkanDevice.h"

class WorldgenSystem
{
public:
	explicit WorldgenSystem(VulkanDevice& device, MeshBatch& batch)
		: device(device), batch(batch) {}

	WorldgenSettings settings;

	void InitSingleChunk(uint32_t seed = 1337u);

	const std::vector<std::unique_ptr<Mesh>>& Meshes() const { return meshes; }

private:
	VulkanDevice& device;
	MeshBatch& batch;
	
	std::unique_ptr<IWorldGenerator> generator;
	std::vector<std::unique_ptr<Mesh>> meshes;
};

#endif // !WORLDGEN_SYSTEM_H
