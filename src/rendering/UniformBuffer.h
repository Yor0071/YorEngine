#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <cstring>

template<typename T>
class UniformBuffer
{
public:
	UniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice)
		: device(device), physicalDevice(physicalDevice)
	{
		VkDeviceSize bufferSize = sizeof(T);

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create uniform buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate uniform buffer memory!");
		}

		vkBindBufferMemory(device, buffer, memory, 0);
	}

	~UniformBuffer()
	{
		if (buffer)
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}

		if (memory)
		{
			vkFreeMemory(device, memory, nullptr);
		}
	}

	void Update(const T& data)
	{
		void* mappedData;
		vkMapMemory(device, memory, 0, sizeof(T), 0, &mappedData);
		std::memcpy(mappedData, &data, sizeof(T));
		vkUnmapMemory(device, memory);
	}

	VkBuffer GetBuffer() const { return buffer; }

private:
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type for uniform buffer");
	}
};

#endif // !UNIFORM_BUFFER_H
