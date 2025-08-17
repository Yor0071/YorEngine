#include "MeshBuild.h"

static void addSkirts(std::vector<Vertex>& V, std::vector<uint32_t>& I, int Vlod, float skirtHeight)
{
	// Helper to map (x, z) index to main grid
	auto idx = [Vlod](int x, int z) { return (uint32_t)(z * Vlod + x); };

	// For each edge, duplicate the edge ring lowered by skirts
	// and connect with two triangles per segment
    auto make_edge = [&](int x0, int z0, int dx, int dz, int count) {
        std::vector<uint32_t> top; top.reserve(count);
        for (int k = 0; k < count; ++k)
        {
			top.push_back(idx(x0 + k * dx, z0 + k * dz));
        }
		std::vector<uint32_t> bot; bot.reserve(count);
		bot.resize(count);

        for (int k = 0; k < count; ++k)
        {
			Vertex v = V[top[k]];
			v.pos.y -= skirtHeight;
			V.push_back(v);
			bot[k] = (uint32_t)(V.size() - 1);
        }

        for (int k = 0; k < count - 1; ++k)
        {
			uint32_t i0 = top[k], i1 = top[k + 1];
			uint32_t j0 = bot[k], j1 = bot[k + 1];

			I.push_back(i0); I.push_back(j0); I.push_back(i1);
			I.push_back(i1); I.push_back(j0); I.push_back(j1);
        }
	};

	// top edge (z = 0), left -> right
	make_edge(0, 0, 1, 0, Vlod);
	// bottom edge (z = Vlod - 1), right -> left
	make_edge(0, Vlod - 1, 1, 0, Vlod);
	// left edge (x = 0), top -> bottom
	make_edge(0, 0, 0, 1, Vlod);
	// right edge (x = Vlod - 1), bottom -> top
	make_edge(Vlod - 1, 0, 0, 1, Vlod);
}

void buildChunkMesh(const IWorldGenerator& gen, const WorldgenSettings& settings, const WorldgenChunkKey& key, std::vector<Vertex>& outV, std::vector<uint32_t>& outI)
{
    const int V = settings.vertsPerSide;
	const int step = 1 << key.lod;
	const int Vlod = (V - 1) / step + 1;

	outV.clear(); outI.clear();
	outV.reserve(Vlod * Vlod + Vlod * 4);
	outI.reserve((Vlod - 1) * (Vlod - 1) * 6 + (Vlod - 1) * 4 * 6);

	const float worldSize = (V - 1) * settings.cellSize;
	const float ox = key.cx * worldSize;
	const float oz = key.cz * worldSize;

	// Generate vertices
    for (int z = 0; z < Vlod; ++z)
    {
        for (int x = 0; x < Vlod; ++x)
        {
			int hx = x * step; int hz = z * step;
			float wx = ox + hx * settings.cellSize;
			float wz = oz + hz * settings.cellSize;
			float wy = settings.heightscale * gen.height(wx, wz) + settings.heightBias;
            
            Vertex v{};
			v.pos = { wx, wy, wz };
            v.color = { 1, 1, 1 };
            v.uv = { wx * 0.05f, wz * 0.05f };
            v.normal = { 0,1,0 };

			outV.push_back(v);
		}
    }

	// Generate indices
	auto idx = [Vlod](int x, int z) { return (uint32_t)(z * Vlod + x); };
    for (int z = 0; z < Vlod - 1; ++z)
    {
        for (int x = 0; x < Vlod - 1; ++x)
        {
            uint32_t i0 = idx(x, z), i1 = idx(x + 1, z), i2 = idx(x, z + 1), i3 = idx(x + 1, z + 1);
            outI.push_back(i0); outI.push_back(i2); outI.push_back(i1);
            outI.push_back(i1); outI.push_back(i2); outI.push_back(i3);
		}
    }
	
    computeNormals(outV, outI);

    // Skirts to hide Lod cracks
    if (settings.skirtHeight > 0.0f)
    {
        addSkirts(outV, outI, Vlod, settings.skirtHeight);
	}

}

void computeNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    std::vector<glm::vec3> acc(vertices.size(), glm::vec3(0));

    for (size_t t = 0; t + 2 < indices.size(); t += 3) 
    {
        uint32_t i0 = indices[t + 0];
        uint32_t i1 = indices[t + 1];
        uint32_t i2 = indices[t + 2];
        const glm::vec3& p0 = vertices[i0].pos;
        const glm::vec3& p1 = vertices[i1].pos;
        const glm::vec3& p2 = vertices[i2].pos;
        glm::vec3 n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        acc[i0] += n; acc[i1] += n; acc[i2] += n;
    }

    for (size_t i = 0; i < vertices.size(); ++i) 
    {
        glm::vec3 n = acc[i];
        if (glm::dot(n, n) < 1e-8f) n = glm::vec3(0, 1, 0);
        vertices[i].normal = glm::normalize(n);
    }
}
