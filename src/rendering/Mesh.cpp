#include "Mesh.h"

Mesh::Mesh(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    vertexBuffer = std::make_unique<VertexBuffer>(device, vertices.data(), sizeof(Vertex) * vertices.size());
    indexBuffer = std::make_unique<IndexBuffer>(device, indices.data(), sizeof(uint32_t) * indices.size(), static_cast<uint32_t>(indices.size()));
    indexCount = static_cast<uint32_t>(indices.size());
}

void Mesh::Bind(VkCommandBuffer commandBuffer) const
{
	vertexBuffer->Bind(commandBuffer);
	indexBuffer->Bind(commandBuffer);
}

void Mesh::Draw(VkCommandBuffer commandBuffer) const
{
	vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}