#include "WorldgenSystem.h"

void WorldgenSystem::InitSingleChunk(uint32_t seed) 
{
    auto gen = std::make_unique<DomainWarpedRidged>();
    gen->base.seed = seed; gen->warpA.seed = seed * 3u; gen->warpB.seed = seed * 5u;
    generator = std::move(gen);

    WorldgenChunkKey key{ 0,0,0 };
    std::vector<Vertex> v; std::vector<uint32_t> i;
    buildChunkMesh(*generator, settings, key, v, i);

    MeshBatch::MeshRange range{};
    batch.UploadMeshToGPU(device, v, i, range);
    const auto& uploaded = batch.GetLastUploadedMesh();
    meshes.emplace_back(std::make_unique<Mesh>(uploaded.vertexBuffer, uploaded.vertexMemory, uploaded.indexBuffer, uploaded.indexMemory, range));
}