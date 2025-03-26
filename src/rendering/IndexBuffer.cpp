#include "IndexBuffer.h"
#include "VulkanDevice.h"

#include <stdexcept>
#include <cstring>

IndexBuffer::IndexBuffer(VkDevice device, uint32_t memoryTypeIndex, const void* indexData, size_t size, uint32_t indexCount)
	: device(device), indexCount(indexCount)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create index buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate index buffer memory!");
	}

	vkBindBufferMemory(device, buffer, memory, 0);

	void* mappedData;
	vkMapMemory(device, memory, 0, size, 0, &mappedData);
	std::memcpy(mappedData, indexData, size);
	vkUnmapMemory(device, memory);
}

IndexBuffer::~IndexBuffer()
{
	if (buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, buffer, nullptr);
		buffer = VK_NULL_HANDLE;
	}

	if (memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, memory, nullptr);
		memory = VK_NULL_HANDLE;
	}
}

void IndexBuffer::Bind(VkCommandBuffer commandBuffer) const
{
	vkCmdBindIndexBuffer(commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT32);
}