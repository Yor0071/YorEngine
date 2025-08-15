#include "TerrainComponent.h"
#include "../utils/PerlinNoise.h"

static float fbm2D(PerlinNoise& n, float x, float z, int octaves, float lacunarity, float gain)
{
    float amp = 1.0f, freq = 1.0f, sum = 0.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += n.noise(x * freq, z * freq) * amp;
        norm += amp;
        amp *= gain;
        freq *= lacunarity;
    }
    return (norm > 0.0f) ? (sum / norm) : 0.0f;
}

void GenerateTerrainMesh(TerrainComponent& terrain)
{
    PerlinNoise noise;

    terrain.vertices.clear();
    terrain.indices.clear();
    terrain.vertices.reserve(static_cast<size_t>(terrain.width) * terrain.depth);
    terrain.indices.reserve(static_cast<size_t>(terrain.width - 1) * (terrain.depth - 1) * 6);

    // -------- vertices --------
    for (int z = 0; z < terrain.depth; ++z) {
        for (int x = 0; x < terrain.width; ++x) {
            float wx = static_cast<float>(x) * terrain.cellSize;
            float wz = static_cast<float>(z) * terrain.cellSize;

            float sx = static_cast<float>(x) * terrain.noiseScale;
            float sz = static_cast<float>(z) * terrain.noiseScale;

            float h = fbm2D(noise, sx, sz, 5, 2.0f, 0.5f);
            float wy = h * terrain.amplitude;

            Vertex v{};
            v.pos = glm::vec3(wx, wy, wz);
            v.uv = glm::vec2(
                terrain.width > 1 ? (float)x / (terrain.width - 1) : 0.0f,
                terrain.depth > 1 ? (float)z / (terrain.depth - 1) : 0.0f
            );
            v.color = glm::vec3(1.0f);      // not used if textured
            v.normal = glm::vec3(0, 1, 0);    // temp; fixed below
            terrain.vertices.push_back(v);
        }
    }

    // -------- indices (CCW) --------
    for (int z = 0; z < terrain.depth - 1; ++z) {
        for (int x = 0; x < terrain.width - 1; ++x) {
            int i = x + z * terrain.width;
            terrain.indices.push_back(i);
            terrain.indices.push_back(i + terrain.width);
            terrain.indices.push_back(i + 1);

            terrain.indices.push_back(i + 1);
            terrain.indices.push_back(i + terrain.width);
            terrain.indices.push_back(i + terrain.width + 1);
        }
    }

    // -------- smooth normals --------
    // accumulate face normals
    std::vector<glm::vec3> acc(terrain.vertices.size(), glm::vec3(0.0f));
    for (size_t t = 0; t < terrain.indices.size(); t += 3) {
        uint32_t i0 = terrain.indices[t + 0];
        uint32_t i1 = terrain.indices[t + 1];
        uint32_t i2 = terrain.indices[t + 2];

        const glm::vec3& p0 = terrain.vertices[i0].pos;
        const glm::vec3& p1 = terrain.vertices[i1].pos;
        const glm::vec3& p2 = terrain.vertices[i2].pos;

        glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        acc[i0] += n; acc[i1] += n; acc[i2] += n;
    }
    for (size_t i = 0; i < terrain.vertices.size(); ++i) {
        glm::vec3 n = acc[i];
        if (glm::dot(n, n) < 1e-8f) n = glm::vec3(0, 1, 0);
        terrain.vertices[i].normal = glm::normalize(n);
    }
}