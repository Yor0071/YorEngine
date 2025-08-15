#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>

#include "QueueFamilyIndices.h"
#include "VulkanDepthBuffer.h"
#include "ThreadCommandPool.h"

class VulkanSwapChain;

class VulkanDevice
{
public:
	VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
	~VulkanDevice();

	void RecreateSwapChain();
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool, VkQueue graphicsQueue);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void CopyBufferToImage(VkBuffer srcBuffer, VkDeviceSize bufferOffset, VkImage image, uint32_t width, uint32_t height);

	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
	VkDevice GetLogicalDevice() const { return logicalDevice; }
	VulkanSwapChain* GetSwapChain() const { return swapChain.get(); }
	VulkanDepthBuffer* GetDepthBuffer() const { return depthBuffer.get(); }
	VkCommandPool GetCommandPool() const { return commandPool; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue; }
	VkQueue GetPresentQueue() const { return presentQueue; }
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	inline std::string GetAssetBasePath() const {
		return "../assets/models/Main.1_Sponza/"; // or wherever Sponza's textures are located
	}
	ThreadCommandPool* GetThreadCommandPool() const { return threadCommandPool.get(); }

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	std::mutex queueSubmitMutex;

	VkResult SubmitGraphicsLocked(const VkSubmitInfo* submits, uint32_t count, VkFence fence = VK_NULL_HANDLE);
	void WaitGraphicsIdleLocked();
	VkResult PresentLocked(const VkPresentInfoKHR* presentInfo);
	

	// Test
	//VulkanDevice(const VulkanDevice&) = delete;
	//VulkanDevice& operator=(const VulkanDevice&) = delete;
private:
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	void CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	void CreateCommandPool();


	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkCommandPool commandPool;


	std::unique_ptr<VulkanSwapChain> swapChain;
	std::unique_ptr<VulkanDepthBuffer> depthBuffer;
	std::unique_ptr<ThreadCommandPool> threadCommandPool;
};

#endif // !VULKAN_DEVICE_H