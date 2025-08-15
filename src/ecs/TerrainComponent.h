#ifndef TERRAIN_COMPONENT_H
#define TERRAIN_COMPONENT_H

#include <vector>
#include <glm/glm.hpp>
#include "../rendering/Vertex.h"

struct TerrainComponent
{
	int width = 1024;
	int depth = 1024;
	
	float amplitude = 20.0f;
	float noiseScale = 0.01f;
	float cellSize = 0.5f;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

void GenerateTerrainMesh(TerrainComponent& terrain);

#endif // !TERRAIN_COMPONENT_H
