#include "WorldgenSystem.h"

void WorldgenSystem::InitSingleChunk(uint32_t seed) 
{
	auto gen = std::make_unique<DomainWarpedRidged>();
	gen->base.seed = seed;
	gen->warpA.seed = seed * 3u;
	gen->warpB.seed = seed * 5u;
	generator = std::move(gen);

	// Build just (0, 0)
	EnsureChunk(0, 0);
}

void WorldgenSystem::Update(const glm::vec3& camPos)
{
	if (!generator) { InitSingleChunk(1337u); }
	const float ws = ChunkWorldSize();
	const int ccx = (int)std::floor(camPos.x / ws);
	const int ccz = (int)std::floor(camPos.z / ws);

	for (int dz= -viewRadius; dz <= viewRadius; ++dz)
	{
		for (int dx = -viewRadius; dx <= viewRadius; ++dx)
		{
			EnsureChunk(ccx + dx, ccz + dz);
		}
	}
}

void WorldgenSystem::BuildVisibleSet(const glm::mat4& view, const glm::mat4& proj)
{
	visibleMeshes.clear();
	glm::mat4 vp = proj * view; // world -> clip
	glm::vec4 planes[6];
	ExtractFrustumPlanes(vp, planes);

	for (auto& kv : chunks)
	{
		Chunk* c = kv.second.get();
		if (!c || !c->mesh) continue;
		if (AABBIntersectsFrustum(c->bounds, planes))
		{
			visibleMeshes.push_back(c->mesh.get());
		}
	}
}

void WorldgenSystem::EnsureChunk(int cx, int cz)
{
	std::pair<int, int> key{ cx, cz };
	if (chunks.find(key) != chunks.end()) return;

	WorldgenChunkKey ck{ cx, cz, 0 };
	std::vector<Vertex> v; std::vector<uint32_t> i;
	buildChunkMesh(*generator, settings, ck, v, i);

	// Upload
	MeshBatch::MeshRange range{};
	batch.UploadMeshToGPU(device, v, i, range);
	const auto& uploaded = batch.GetLastUploadedMesh();

	auto ch = std::make_unique<Chunk>();
	ch->key = ck;
	ch->mesh = std::make_unique<Mesh>(uploaded.vertexBuffer, uploaded.vertexMemory, uploaded.indexBuffer, uploaded.indexMemory, range);

	// Compute AAB from vertices
	glm::vec3 mn(std::numeric_limits<float>::max());
	glm::vec3 mx(-std::numeric_limits<float>::max());
	for (const auto& vert : v)
	{
		mn = glm::min(mn, vert.pos);
		mx = glm::max(mx, vert.pos);
	}
	ch->bounds = { mn, mx };

	chunks.emplace(key, std::move(ch));
}

static inline void normalizePlane(glm::vec4& p)
{
	float invLen = 1.0f / std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z + 1e-20f);
	p *= invLen;
}

void WorldgenSystem::ExtractFrustumPlanes(const glm::mat4& m, glm::vec4 out[6]) 
{
	// GLM is column-major: m[col][row]
	// Left   : row4 + row1
	out[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
	// Right  : row4 - row1
	out[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
	// Bottom : row4 + row2
	out[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
	// Top    : row4 - row2
	out[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
	// Near   : row4 + row3
	out[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]);
	// Far    : row4 - row3
	out[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);

	for (int i = 0;i < 6;++i) normalizePlane(out[i]);
}

bool WorldgenSystem::AABBIntersectsFrustum(const AABB& b, const glm::vec4 planes[6]) {
	for (int i = 0;i < 6;++i) {
		const glm::vec3 n = glm::vec3(planes[i]);
		const float d = planes[i].w;
		// Support point (p-vertex) on the AABB toward the plane normal
		glm::vec3 p = {
			n.x >= 0 ? b.max.x : b.min.x,
			n.y >= 0 ? b.max.y : b.min.y,
			n.z >= 0 ? b.max.z : b.min.z
		};
		if (glm::dot(n, p) + d < 0.0f) return false; // completely outside
	}
	return true;
}