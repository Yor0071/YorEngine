#ifndef MESH_BATCH_H
#define MESH_BATCH_H

#include <vector>

#include "Vertex.h"
#include "VulkanDevice.h"

class MeshBatch
{
public:
	struct MeshRange {
		uint32_t indexOffset;
		uint32_t indexCount;
		uint32_t vertexOffset;
	};

	struct GpuMesh
	{
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
		VkBuffer indexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory indexMemory = VK_NULL_HANDLE;
		MeshRange range;
	};

	std::vector<GpuMesh> gpuMeshes;

	MeshBatch();
	~MeshBatch();

	MeshRange AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	void UploadToGPU(VulkanDevice& device);
	void Destroy(VkDevice device);
	void UploadMeshToGPU(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, MeshRange& outRange);

	void BindBuffers(VkCommandBuffer commandBuffer) const;

	const GpuMesh& GetLastUploadedMesh() const { return gpuMeshes.back(); }

	VkBuffer GetVertexBuffer() const { return vertexBuffer; };
	VkBuffer GetIndexBuffer() const { return indexBuffer; };	
private:
	std::vector<Vertex> allVertices;
	std::vector<uint32_t> allIndices;

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

	bool uploaded = false;
};

#endif // !MESH_BATCH_H
