#include "TerrainComponent.h"
#include "../utils/PerlinNoise.h"

void GenerateTerrainMesh(TerrainComponent& terrain) {
    PerlinNoise noise;

    terrain.vertices.clear();
    terrain.indices.clear();
	terrain.vertices.reserve(static_cast<size_t>(terrain.width) * terrain.depth);
	terrain.indices.reserve(static_cast<size_t>(terrain.width - 1) * (terrain.depth - 1) * 6);

    for (int z = 0; z < terrain.depth; ++z) {
        for (int x = 0; x < terrain.width; ++x) {
            // Frequency
            float xf = static_cast<float>(x) * terrain.noiseScale;
            float zf = static_cast<float>(z) * terrain.noiseScale;

            // Height in world units
            float y = noise.noise(xf, zf) * terrain.amplitude;

            Vertex v;
            v.pos = glm::vec3(x* terrain.cellSize, y, z * terrain.cellSize);

            // Basic uv mapping
            v.uv = glm::vec2(
                terrain.width > 1 ? (float)x / (terrain.width - 1) : 0.0f,
                terrain.depth > 1 ? (float)z / (terrain.depth - 1) : 0.0f
            );
            
            // If vertex has color and don't set it elsewhere
            v.color = glm::vec3(0.4f, 0.5f, 1.0f);  // grass-like green

            terrain.vertices.push_back(v);
        }
    }

	// Triangles
    for (int z = 0; z < terrain.depth - 1; ++z) {
        for (int x = 0; x < terrain.width - 1; ++x) {
            int i = x + z * terrain.width;

            // Winding: i, i+width, i+1 and i+1, i+width, i+width+1
            // This is CCW if Y is up and you look from above.
            terrain.indices.push_back(i);
            terrain.indices.push_back(i + terrain.width);
            terrain.indices.push_back(i + 1);

            terrain.indices.push_back(i + 1);
            terrain.indices.push_back(i + terrain.width);
            terrain.indices.push_back(i + terrain.width + 1);
        }
    }
}