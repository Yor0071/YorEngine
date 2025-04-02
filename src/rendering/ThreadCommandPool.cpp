#include "ThreadCommandPool.h"

ThreadCommandPool::ThreadCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex)
	: device(logicalDevice), queueFamilyIndex(queueFamilyIndex)
{
}

ThreadCommandPool::~ThreadCommandPool()
{
	for (auto& pair : threadPools)
	{
		vkDestroyCommandPool(device, pair.second, nullptr);
	}
}

VkCommandPool ThreadCommandPool::GetOrCreatePoolForCurrentThread()
{
	std::lock_guard<std::mutex> lock(mutex);
	std::thread::id threadId = std::this_thread::get_id();

	auto it = threadPools.find(threadId);
	if (it != threadPools.end())
	{
		return it->second;
	}

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool commandPool;
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}

	threadPools[threadId] = commandPool;
	return commandPool;
}