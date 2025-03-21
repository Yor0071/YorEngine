#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

class IndexBuffer
{
public:
	IndexBuffer(VulkanDevice& device, const void* indexData, size_t size, uint32_t indexCount);
	~IndexBuffer();

	VkBuffer GetBuffer() const { return buffer; }
	size_t GetIndexCount() const { return indexCount; }

private:
	VulkanDevice& device;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	size_t indexCount = 0;
};

#endif // !INDEX_BUFFER_H
