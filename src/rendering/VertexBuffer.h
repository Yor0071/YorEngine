#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include "Vertex.h"

class VulkanDevice;

class VertexBuffer
{
public:
	VertexBuffer(VulkanDevice& device, const void* vertexData, size_t size);
	~VertexBuffer();

	VkBuffer GetBuffer() const { return buffer; }
	//VkDeviceMemory GetBufferMemory() const { return bufferMemory; }
	size_t GetVertexCount() const { return vertexCount; }

	void Bind(VkCommandBuffer commandBuffer) const;

private:
	size_t vertexCount;

	VulkanDevice& device;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

#endif // !VERTEX_BUFFER_H
