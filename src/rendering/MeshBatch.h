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

	MeshBatch();
	~MeshBatch();

	MeshRange AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	void UploadToGPU(VulkanDevice& device);
	void UploadMeshToGPU(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, MeshRange& outRange);
	
	void Destroy(VkDevice device);
	void BindBuffers(VkCommandBuffer commandBuffer) const;

	const GpuMesh& GetLastUploadedMesh() const;
	VkBuffer GetVertexBuffer() const { return vertexBuffer; };
	VkBuffer GetIndexBuffer() const { return indexBuffer; };

	void SetCustomCommandPool(VkCommandPool customPool);

	void Reset();
private:
	std::vector<Vertex> allVertices;
	std::vector<uint32_t> allIndices;
	std::vector<GpuMesh> gpuMeshes;

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

	VkCommandPool overrideCommandPool = VK_NULL_HANDLE;

	bool uploaded = false;
};

#endif // !MESH_BATCH_H
