#include "VertexBuffer.h"
#include "VulkanDevice.h"

#include <stdexcept>
#include <cstring>

VertexBuffer::VertexBuffer(VulkanDevice& device, const void* vertexData, size_t size)
	: device(device)
{
	vertexCount = size / sizeof(Vertex);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device.GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device.GetLogicalDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(device.GetLogicalDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(device.GetLogicalDevice(), buffer, memory, 0);

	void* mappedData;
	vkMapMemory(device.GetLogicalDevice(), memory, 0, size, 0, &mappedData);
	std::memcpy(mappedData, vertexData, size);
	vkUnmapMemory(device.GetLogicalDevice(), memory);
}

VertexBuffer::~VertexBuffer()
{
	if (buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device.GetLogicalDevice(), buffer, nullptr);
	}

	if (memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device.GetLogicalDevice(), memory, nullptr);
	}
}