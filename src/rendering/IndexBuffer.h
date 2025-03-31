#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

class IndexBuffer
{
public:
	IndexBuffer(VkDevice device, uint32_t memoryTypeIndex, const void* indexData, size_t size, uint32_t indexCount);
	~IndexBuffer();

	VkBuffer GetBuffer() const { return buffer; }
	size_t GetIndexCount() const { return indexCount; }

	void Bind(VkCommandBuffer commandBuffer) const;
private:
	VkDevice device = VK_NULL_HANDLE;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	size_t indexCount = 0;
};

#endif // !INDEX_BUFFER_H
