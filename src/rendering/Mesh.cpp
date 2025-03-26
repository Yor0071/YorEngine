#include "Mesh.h"

Mesh::Mesh(VkDevice device, uint32_t memoryTypeIndex, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    vertexBuffer = std::make_unique<VertexBuffer>(
        device,
        memoryTypeIndex,
        vertices.data(),
        sizeof(Vertex) * vertices.size()
    );

    indexBuffer = std::make_unique<IndexBuffer>(
        device,
        memoryTypeIndex,
        indices.data(),
        sizeof(uint32_t) * indices.size(),
        static_cast<uint32_t>(indices.size())
    );

    indexCount = static_cast<uint32_t>(indices.size());
}
Mesh::~Mesh()
{
	Destroy();
}

void Mesh::Destroy() {
    vertexBuffer.reset();
    indexBuffer.reset();
    indexCount = 0;
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