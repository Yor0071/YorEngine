#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include "Vertex.h"

class VulkanDevice;

class VertexBuffer
{
public:
	VertexBuffer(VkDevice device,uint32_t memoryTypeIndex, const void* vertexData, size_t size);
	~VertexBuffer();

	void Bind(VkCommandBuffer commandBuffer) const;
	
	VkBuffer GetBuffer() const { return buffer; }
	size_t GetVertexCount() const { return vertexCount; }

private:
	size_t vertexCount = 0;

	VkDevice device = VK_NULL_HANDLE;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

#endif // !VERTEX_BUFFER_H
