#ifndef MESH_H
#define MESH_H

#include <memory>
#include <glm/glm.hpp>

#include "IndexBuffer.h"
#include "VertexBuffer.h"

class Mesh
{
public:
	Mesh(VkDevice device, uint32_t memoryTypeIndex, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	~Mesh();

	void Destroy();

	void Bind(VkCommandBuffer commandBuffer) const;
	void Draw(VkCommandBuffer commandBuffer) const;

	uint32_t GetIndexCount() const { return indexCount; }
private:
	//VulkanDevice& device;

	std::unique_ptr<VertexBuffer> vertexBuffer;
	std::unique_ptr<IndexBuffer> indexBuffer;
	//VkDeviceMemory vertexMemory;
	//VkDeviceMemory indexMemory;

	uint32_t indexCount = 0;

};

#endif // !MESH_H