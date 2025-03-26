#include "VertexBuffer.h"
#include "VulkanDevice.h"

#include <stdexcept>
#include <cstring>

VertexBuffer::VertexBuffer(VkDevice device, uint32_t memoryTypeIndex, const void* vertexData, size_t size)
	: device(device)
{
	vertexCount = size / sizeof(Vertex);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(device, buffer, memory, 0);

	void* mappedData;
	vkMapMemory(device, memory, 0, size, 0, &mappedData);
	std::memcpy(mappedData, vertexData, size);
	vkUnmapMemory(device, memory);
}

VertexBuffer::~VertexBuffer()
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

void VertexBuffer::Bind(VkCommandBuffer commandBuffer) const
{
	VkBuffer vertexBuffers[] = { buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}