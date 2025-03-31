#include "MeshBatch.h"
#include <stdexcept>
#include <cstring>
#include <iostream>

MeshBatch::MeshBatch()
{
}

MeshBatch::~MeshBatch()
{
}

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

	// === VERTEX BUFFER ===
	VkDeviceSize vertexBufferSize = sizeof(Vertex) * allVertices.size();

	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingMemory;
	device.CreateBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexStagingBuffer,
		vertexStagingMemory
	);

	void* vertexData;
	vkMapMemory(logicalDevice, vertexStagingMemory, 0, vertexBufferSize, 0, &vertexData);
	memcpy(vertexData, allVertices.data(), static_cast<size_t>(vertexBufferSize));
	vkUnmapMemory(logicalDevice, vertexStagingMemory);

	device.CreateBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory
	);

	device.CopyBuffer(vertexStagingBuffer, vertexBuffer, vertexBufferSize, commandPool, graphicsQueue);

	vkDestroyBuffer(logicalDevice, vertexStagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexStagingMemory, nullptr);

	// === INDEX BUFFER ===
	VkDeviceSize indexBufferSize = sizeof(uint32_t) * allIndices.size();

	VkBuffer indexStagingBuffer;
	VkDeviceMemory indexStagingMemory;
	device.CreateBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		indexStagingBuffer,
		indexStagingMemory
	);

	void* indexData;
	vkMapMemory(logicalDevice, indexStagingMemory, 0, indexBufferSize, 0, &indexData);
	memcpy(indexData, allIndices.data(), static_cast<size_t>(indexBufferSize));
	vkUnmapMemory(logicalDevice, indexStagingMemory);

	device.CreateBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory
	);

	device.CopyBuffer(indexStagingBuffer, indexBuffer, indexBufferSize, commandPool, graphicsQueue);

	vkDestroyBuffer(logicalDevice, indexStagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, indexStagingMemory, nullptr);

	// Cleanup CPU-side mesh data
	allVertices.clear();
	allIndices.clear();

	std::cout << "[MeshBatch] Upload complete (" << vertexBufferSize / (1024.0 * 1024.0)
		<< " MB vertices, " << indexBufferSize / (1024.0 * 1024.0) << " MB indices)\n";
}

void MeshBatch::UploadMeshToGPU(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, MeshRange& outRange)
{
	std::cout << "[MeshBatch] Uploading mesh: " << vertices.size() << " vertices, " << indices.size() << " indices\n";

	if (vertices.empty() || indices.empty()) {
		throw std::runtime_error("Attempted to upload mesh with no vertices or indices.");
	}

	VkDevice logicalDevice = device.GetLogicalDevice();
	VkCommandPool commandPool = device.GetCommandPool();
	VkQueue graphicsQueue = device.GetGraphicsQueue();

	VkBuffer vertexBuffer, indexBuffer;
	VkDeviceMemory vertexMemory, indexMemory;

	// === VERTEX STAGING ===
	VkDeviceSize vertexSize = sizeof(Vertex) * vertices.size();

	VkBuffer vertexStaging;
	VkDeviceMemory vertexStagingMem;
	device.CreateBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexStaging, vertexStagingMem);

	void* vertexData;
	vkMapMemory(logicalDevice, vertexStagingMem, 0, vertexSize, 0, &vertexData);
	memcpy(vertexData, vertices.data(), (size_t)vertexSize);
	vkUnmapMemory(logicalDevice, vertexStagingMem);

	device.CreateBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexMemory);

	device.CopyBuffer(vertexStaging, vertexBuffer, vertexSize, commandPool, graphicsQueue);

	vkDestroyBuffer(logicalDevice, vertexStaging, nullptr);
	vkFreeMemory(logicalDevice, vertexStagingMem, nullptr);

	// === INDEX STAGING ===
	VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

	VkBuffer indexStaging;
	VkDeviceMemory indexStagingMem;
	device.CreateBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		indexStaging, indexStagingMem);

	void* indexData;
	vkMapMemory(logicalDevice, indexStagingMem, 0, indexSize, 0, &indexData);
	memcpy(indexData, indices.data(), (size_t)indexSize);
	vkUnmapMemory(logicalDevice, indexStagingMem);

	device.CreateBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexMemory);

	device.CopyBuffer(indexStaging, indexBuffer, indexSize, commandPool, graphicsQueue);

	vkDestroyBuffer(logicalDevice, indexStaging, nullptr);
	vkFreeMemory(logicalDevice, indexStagingMem, nullptr);

	// Save the mesh reference
	outRange.vertexOffset = 0;
	outRange.indexOffset = 0;
	outRange.indexCount = static_cast<uint32_t>(indices.size());

	GpuMesh mesh;
	mesh.vertexBuffer = vertexBuffer;
	mesh.vertexMemory = vertexMemory;
	mesh.indexBuffer = indexBuffer;
	mesh.indexMemory = indexMemory;
	mesh.range = outRange;
	gpuMeshes.push_back(mesh);
}


void MeshBatch::Destroy(VkDevice device)
{
	for (const auto& mesh : gpuMeshes) {
		if (mesh.vertexBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
		if (mesh.vertexMemory != VK_NULL_HANDLE)
			vkFreeMemory(device, mesh.vertexMemory, nullptr);
		if (mesh.indexBuffer != VK_NULL_HANDLE)
			vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
		if (mesh.indexMemory != VK_NULL_HANDLE)
			vkFreeMemory(device, mesh.indexMemory, nullptr);
	}
	gpuMeshes.clear();
}

void MeshBatch::BindBuffers(VkCommandBuffer commandBuffer) const
{
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
}