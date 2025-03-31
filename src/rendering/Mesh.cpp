#include "Mesh.h"

Mesh::Mesh(VkBuffer vb, VkDeviceMemory vm, VkBuffer ib, VkDeviceMemory im, const MeshBatch::MeshRange& r)
	: vertexBuffer(vb), indexBuffer(ib), range(r)
{
}


Mesh::~Mesh()
{
}

void Mesh::Bind(VkCommandBuffer commandBuffer) const
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, range.indexOffset * sizeof(uint32_t), VK_INDEX_TYPE_UINT32);
}

void Mesh::Draw(VkCommandBuffer commandBuffer) const
{
	vkCmdDrawIndexed(commandBuffer, range.indexCount, 1, 0, 0, 0);
}