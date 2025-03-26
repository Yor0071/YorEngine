#ifndef MESH_H
#define MESH_H

#include <memory>
#include <glm/glm.hpp>

#include "IndexBuffer.h"
#include "VertexBuffer.h"

class Mesh
{
public:
	Mesh(VulkanDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	void Bind(VkCommandBuffer commandBuffer) const;
	void Draw(VkCommandBuffer commandBuffer) const;

	uint32_t GetIndexCount() const { return indexCount; }
private:
	std::unique_ptr<VertexBuffer> vertexBuffer;
	std::unique_ptr<IndexBuffer> indexBuffer;

	uint32_t indexCount = 0;
};

#endif // !MESH_H