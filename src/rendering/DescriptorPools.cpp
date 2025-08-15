#include "DescriptorPools.h"

DescriptorPools::DescriptorPools() = default;

DescriptorPools::~DescriptorPools()
{
	if (materialPool)
	{
		vkDestroyDescriptorPool(device, materialPool, nullptr);
	}
}

void DescriptorPools::Init(VkDevice device)
{
	this->device = device;
	VkDescriptorPoolSize size{};
	size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	size.descriptorCount = 4096; // Adjust as needed

	VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	info.maxSets = 4096; // Adjust as needed
	info.poolSizeCount = 1;
	info.pPoolSizes = &size;

	if (vkCreateDescriptorPool(device, &info, nullptr, &materialPool) != VK_SUCCESS)
	{
		throw std::runtime_error("[DescriptorPools] Failed to create material pool");
	}
}

void DescriptorPools::Destroy()
{
	if (materialPool)
	{
		vkDestroyDescriptorPool(device, materialPool, nullptr);
		materialPool = VK_NULL_HANDLE;
	}
}
