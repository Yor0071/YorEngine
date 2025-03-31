#include "MeshBatch.h"
#include <stdexcept>
#include <cstring>
#include <iostream>

MeshBatch::MeshBatch() = default;

MeshBatch::~MeshBatch() = default;

MeshBatch::MeshRange MeshBatch::AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
	std::cout << "[MeshBatch] Adding mesh with " << vertices.size() << " vertices and " << indices.size() << " indices.\n";

	MeshRange range{};
	range.vertexOffset = static_cast<uint32_t>(allVertices.size());
	range.indexOffset = static_cast<uint32_t>(allIndices.size());
	range.indexCount = static_cast<uint32_t>(indices.size());

	allVertices.insert(allVertices.end(), vertices.begin(), vertices.end());

	for (uint32_t index : indices) {
		allIndices.push_back(index + range.vertexOffset);
	}

	return range;
}

void MeshBatch::UploadToGPU(VulkanDevice& device)
{
	if (uploaded) {
		std::cout << "[MeshBatch] Upload skipped (already uploaded)\n";
		return;
	}

	if (allVertices.empty() || allIndices.empty()) {
		std::cout << "[MeshBatch] Upload skipped (no accumulated mesh data)\n";
		return;
	}

	uploaded = true;

	VkDevice logicalDevice = device.GetLogicalDevice();
	VkCommandPool commandPool = device.GetCommandPool();
	VkQueue graphicsQueue = device.GetGraphicsQueue();

	// Helper lambda for creating and filling staging buffers
	auto createStagingBuffer = [&](VkDeviceSize size, const void* data, VkBuffer& stagingBuffer, VkDeviceMemory& stagingMemory) {
		device.CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingMemory);

		void* dst;
		vkMapMemory(logicalDevice, stagingMemory, 0, size, 0, &dst);
		memcpy(dst, data, static_cast<size_t>(size));
		vkUnmapMemory(logicalDevice, stagingMemory);
		};

	// === VERTEX BUFFER ===
	VkDeviceSize vertexSize = sizeof(Vertex) * allVertices.size();
	VkBuffer vertexStaging, indexStaging;
	VkDeviceMemory vertexStagingMem, indexStagingMem;

	createStagingBuffer(vertexSize, allVertices.data(), vertexStaging, vertexStagingMem);

	device.CreateBuffer(vertexSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer, vertexBufferMemory);

	device.CopyBuffer(vertexStaging, vertexBuffer, vertexSize, commandPool, graphicsQueue);

	vkDestroyBuffer(logicalDevice, vertexStaging, nullptr);
	vkFreeMemory(logicalDevice, vertexStagingMem, nullptr);

	// === INDEX BUFFER ===
	VkDeviceSize indexSize = sizeof(uint32_t) * allIndices.size();

	createStagingBuffer(indexSize, allIndices.data(), indexStaging, indexStagingMem);

	device.CreateBuffer(indexSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer, indexBufferMemory);

	device.CopyBuffer(indexStaging, indexBuffer, indexSize, commandPool, graphicsQueue);

	vkDestroyBuffer(logicalDevice, indexStaging, nullptr);
	vkFreeMemory(logicalDevice, indexStagingMem, nullptr);

	allVertices.clear();
	allIndices.clear();

	std::cout << "[MeshBatch] Upload complete (" << vertexSize / (1024.0 * 1024.0)
		<< " MB vertices, " << indexSize / (1024.0 * 1024.0) << " MB indices)\n";
}

void MeshBatch::UploadMeshToGPU(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, MeshRange& outRange)
{
	std::cout << "[MeshBatch] Uploading mesh: " << vertices.size() << " vertices, " << indices.size() << " indices\n";

	if (vertices.empty() || indices.empty()) {
		throw std::runtime_error("[MeshBatch] Attempted to upload empty mesh.");
	}

	VkDevice logicalDevice = device.GetLogicalDevice();
	VkCommandPool commandPool = device.GetCommandPool();
	VkQueue graphicsQueue = device.GetGraphicsQueue();

	VkDeviceSize vertexSize = sizeof(Vertex) * vertices.size();
	VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

	auto createStagingBuffer = [&](VkDeviceSize size, const void* data, VkBuffer& buffer, VkDeviceMemory& memory) {
		device.CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			buffer, memory);

		void* dst;
		vkMapMemory(logicalDevice, memory, 0, size, 0, &dst);
		memcpy(dst, data, static_cast<size_t>(size));
		vkUnmapMemory(logicalDevice, memory);
		};

	VkBuffer vertexStaging, indexStaging;
	VkDeviceMemory vertexStagingMem, indexStagingMem;
	createStagingBuffer(vertexSize, vertices.data(), vertexStaging, vertexStagingMem);
	createStagingBuffer(indexSize, indices.data(), indexStaging, indexStagingMem);

	VkBuffer vertexBuffer, indexBuffer;
	VkDeviceMemory vertexMemory, indexMemory;

	device.CreateBuffer(vertexSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexMemory);
	device.CopyBuffer(vertexStaging, vertexBuffer, vertexSize, commandPool, graphicsQueue);

	device.CreateBuffer(indexSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexMemory);
	device.CopyBuffer(indexStaging, indexBuffer, indexSize, commandPool, graphicsQueue);

	vkDestroyBuffer(logicalDevice, vertexStaging, nullptr);
	vkFreeMemory(logicalDevice, vertexStagingMem, nullptr);
	vkDestroyBuffer(logicalDevice, indexStaging, nullptr);
	vkFreeMemory(logicalDevice, indexStagingMem, nullptr);

	outRange.vertexOffset = 0;
	outRange.indexOffset = 0;
	outRange.indexCount = static_cast<uint32_t>(indices.size());

	GpuMesh mesh{ vertexBuffer, vertexMemory, indexBuffer, indexMemory, outRange };
	gpuMeshes.push_back(mesh);
}

const MeshBatch::GpuMesh& MeshBatch::GetLastUploadedMesh() const
{
	if (gpuMeshes.empty()) {
		throw std::runtime_error("[MeshBatch] No uploaded meshes available.");
	}
	return gpuMeshes.back();
}

void MeshBatch::Reset()
{
	assert(this != nullptr);
	allVertices.clear();
	allIndices.clear();
	uploaded = false;
}

void MeshBatch::Destroy(VkDevice device)
{
	for (const auto& mesh : gpuMeshes) {
		if (mesh.vertexBuffer) vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
		if (mesh.vertexMemory) vkFreeMemory(device, mesh.vertexMemory, nullptr);
		if (mesh.indexBuffer) vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
		if (mesh.indexMemory) vkFreeMemory(device, mesh.indexMemory, nullptr);
	}
	gpuMeshes.clear();

	if (vertexBuffer) vkDestroyBuffer(device, vertexBuffer, nullptr);
	if (vertexBufferMemory) vkFreeMemory(device, vertexBufferMemory, nullptr);
	if (indexBuffer) vkDestroyBuffer(device, indexBuffer, nullptr);
	if (indexBufferMemory) vkFreeMemory(device, indexBufferMemory, nullptr);

	vertexBuffer = VK_NULL_HANDLE;
	vertexBufferMemory = VK_NULL_HANDLE;
	indexBuffer = VK_NULL_HANDLE;
	indexBufferMemory = VK_NULL_HANDLE;
	uploaded = false;
}

void MeshBatch::BindBuffers(VkCommandBuffer commandBuffer) const
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}