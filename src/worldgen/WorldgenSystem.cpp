#include "WorldgenSystem.h"

void WorldgenSystem::InitSingleChunk(uint32_t seed) 
{
	auto gen = std::make_unique<DomainWarpedRidged>();
	gen->base.seed = seed;
	gen->warpA.seed = seed * 3u;
	gen->warpB.seed = seed * 5u;
	generator = std::move(gen);
	EnsureChunkLOD_Immediate(0, 0, 0);
}

void WorldgenSystem::StartWorkers() 
{
	stop = false;
	worker = std::thread(&WorldgenSystem::WorkerLoop, this);
}

void WorldgenSystem::Shutdown() 
{
	stop = true;
	cv.notify_all();
	if (worker.joinable()) 
	{
		worker.join();
	}
}

static inline int ringLOD(int dx, int dz, const LODConfig& cfg)
{
	int d = std::max(std::abs(dx), std::abs(dz));
	if (d <= cfg.fullDetailRings) return 0;               // center + rings at LOD0
	int L = d - cfg.fullDetailRings;                      // each ring increases by 1
	return std::min(L, cfg.maxLOD);
}

static inline int chooseLOD(const glm::vec2& camXZ, const glm::vec2& chunkCenterXZ, const LODConfig& cfg)
{
	float dist = glm::length(camXZ - chunkCenterXZ);
	float threshold = cfg.lodDistance;
	int l = 0;
	while (l < cfg.maxLOD && dist > threshold)
	{
		threshold *= cfg.lodBase;
		++l;
	}
	return l;
}

void WorldgenSystem::Update(const glm::vec3& camPos)
{
	if (!generator) { InitSingleChunk(1337u); }

	const float ws = ChunkWorldSize();
	const int ccx = (int)std::floor(camPos.x / ws);
	const int ccz = (int)std::floor(camPos.z / ws);

	const int rings = std::max(1, lod.fullDetailRings + lod.maxLOD);

	for (int dz = -rings; dz <= rings; ++dz)
		for (int dx = -rings; dx <= rings; ++dx)
		{
			int cx = ccx + dx, cz = ccz + dz;
			int L = ringLOD(dx, dz, lod);

			// quick skip if already correct on GPU
			auto itc = chunks.find({ cx, cz });
			if (itc != chunks.end() && itc->second && itc->second->mesh && itc->second->lod == L) {
				continue;
			}

			float cxw = (cx + 0.5f) * ws, bzw = (cz + 0.5f) * ws;
			float dist = std::hypot(cxw - camPos.x, bzw - camPos.z);
			EnqueueJob(cx, cz, L, dist);
		}
}

void WorldgenSystem::EnqueueJob(int cx, int cz, int L, float priority)
{
	// already on GPU at desired LOD?
	auto itc = chunks.find({ cx, cz });
	if (itc != chunks.end() && itc->second && itc->second->mesh && itc->second->lod == L) return;

	std::unique_lock<std::mutex> lk(mtx);

	// if there’s already a job for this tile, upgrade it (smaller L is finer)
	for (auto it = jobQueue.begin(); it != jobQueue.end(); ++it) {
		if (it->cx == cx && it->cz == cz) {
			if (L < it->lod) it->lod = L; // upgrade to finer LOD
			it->priority = std::min(it->priority, priority);
			// fix queuedKeys set: old key out, new key in
			long long oldKey = ((long long)cx << 32) ^ (unsigned long long)(unsigned)cz ^ ((long long)it->lod << 8);
			queuedKeys.erase(oldKey); // remove with old lod
			long long newKey = ((long long)cx << 32) ^ (unsigned long long)(unsigned)cz ^ ((long long)L << 8);
			queuedKeys.insert(newKey);
			std::sort(jobQueue.begin(), jobQueue.end(),
				[](const Job& a, const Job& b) { return a.priority > b.priority; });
			return;
		}
	}

	// normal dedupe by (cx,cz,lod)
	long long key = ((long long)cx << 32) ^ (unsigned long long)(unsigned)cz ^ ((long long)L << 8);
	if (queuedKeys.find(key) != queuedKeys.end()) return;

	queuedKeys.insert(key);
	jobQueue.push_back({ cx, cz, L, priority });
	std::sort(jobQueue.begin(), jobQueue.end(),
		[](const Job& a, const Job& b) { return a.priority > b.priority; });
	cv.notify_one();
}

void WorldgenSystem::WorkerLoop()
{
	while (!stop)
	{
		Job job;
		{
			std::unique_lock<std::mutex> lk(mtx);
			cv.wait(lk, [&] { return stop || !jobQueue.empty(); });
			if (stop) break;
			job = jobQueue.back();
			jobQueue.pop_back();
			long long key = ((long long)job.cx << 32) ^ ((unsigned long long)(unsigned)job.cz) ^ ((long long)job.lod << 8);
			queuedKeys.erase(key);
		}
		
		// Build on background thread
		WorldgenChunkKey ck{ job.cx, job.cz, job.lod };
		std::vector<Vertex> v; std::vector<uint32_t> i;
		buildChunkMesh(*generator, settings, ck, v, i);

		// AABB (include skirts)
		glm::vec3 mn(std::numeric_limits<float>::max());
		glm::vec3 mx(-std::numeric_limits<float>::max());
		for (const auto& vert : v)
		{
			mn = glm::min(mn, vert.pos);
			mx = glm::max(mx, vert.pos);
		}
		mn.y -= settings.skirtHeight;

		// Push result
		std::unique_lock<std::mutex> lk(mtx);
		ready.push_back({ job.cx, job.cz, job.lod, std::move(v), std::move(i), { mn, mx } });
	}
}

void WorldgenSystem::PumpUploads(int budgetMeshesPerFrame)
{
	// Upload a few CPU results to GPU on the main thread
	std::unique_lock<std::mutex> lk(mtx);
	int uploaded = 0;

	while (uploaded < budgetMeshesPerFrame && !ready.empty())
	{
		Result r = std::move(ready.front());
		ready.pop_front();
		lk.unlock();

		// Upload mesh to GPU
		MeshBatch::MeshRange range{};
		batch.UploadMeshToGPU(device, r.v, r.i, range);
		const auto& up = batch.GetLastUploadedMesh();

		std::pair<int, int> key{ r.cx, r.cz };
		auto& ch = chunks[key];
		if (!ch) 
		{
			ch = std::make_unique<Chunk>();
		}
		ch->cx = r.cx; ch->cz = r.cz; ch->lod = r.lod;
		ch->mesh = std::make_unique<Mesh>(up.vertexBuffer, up.vertexMemory, up.indexBuffer, up.indexMemory, range);
		ch->bounds = r.bounds; // AABB with skirts

		++uploaded;
		lk.lock(); // lock again for next iteration
	}
}

void WorldgenSystem::EnsureChunkLOD_Immediate(int cx, int cz, int desiredLOD) 
{
	WorldgenChunkKey ck{ cx, cz, desiredLOD };
	std::vector<Vertex> v; std::vector<uint32_t> i;
	buildChunkMesh(*generator, settings, ck, v, i);

	MeshBatch::MeshRange range{};
	batch.UploadMeshToGPU(device, v, i, range);
	const auto& uploaded = batch.GetLastUploadedMesh();

	std::pair<int, int> key{ cx,cz };
	auto& ch = chunks[key];
	if (!ch)
	{
		ch = std::make_unique<Chunk>();
	}
	ch->cx = cx; ch->cz = cz; ch->lod = desiredLOD;
	ch->mesh = std::make_unique<Mesh>(uploaded.vertexBuffer, uploaded.vertexMemory, uploaded.indexBuffer, uploaded.indexMemory, range);

	glm::vec3 mn(std::numeric_limits<float>::max());
	glm::vec3 mx(-std::numeric_limits<float>::max());
	for (const auto& vert : v) 
	{ 
		mn = glm::min(mn, vert.pos); mx = glm::max(mx, vert.pos); 
	}
	mn.y -= settings.skirtHeight; ch->bounds = { mn, mx };
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