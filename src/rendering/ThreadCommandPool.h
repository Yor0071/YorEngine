#ifndef THREAD_COMMAND_POOL_H
#define THREAD_COMMAND_POOL_H

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <thread>
#include <mutex>

class ThreadCommandPool
{
public:
	ThreadCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex);
	~ThreadCommandPool();

	VkCommandPool GetOrCreatePoolForCurrentThread();
private:
	VkDevice device;
	uint32_t queueFamilyIndex;
	std::mutex mutex;
	std::unordered_map<std::thread::id, VkCommandPool> threadPools;
};

#endif // !THREAD_COMMAND_POOL_H