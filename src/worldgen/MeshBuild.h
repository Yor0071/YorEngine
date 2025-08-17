#ifndef MESH_BUILD_H
#define MESH_BUILD_H

#include <vector>
#include <glm/glm.hpp>
#include <cmath>

#include "../rendering/Vertex.h"
#include "IWorldGenerator.h"

struct WorldgenSettings
{
	int vertsPerSide = 129;
	float cellSize = 1.0f;
	float skirtHeight = 0.5f;
	float heightscale = 1.0f;
	float heightBias = 0.0f;
};

struct WorldgenChunkKey { int cx = 0, cz = 0, lod = 0; };

void buildChunkMesh(const IWorldGenerator& gen, const WorldgenSettings& settings, const WorldgenChunkKey& key, std::vector<Vertex>& outV, std::vector<uint32_t>& outI);
void computeNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

#endif // !MESH_BUILD_H
