#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <memory>
#include "QueueFamilyIndices.h"
#include "VulkanDepthBuffer.h"

class VulkanSwapChain;

class VulkanDevice
{
public:
	VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
	~VulkanDevice();

	void RecreateSwapChain();
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool, VkQueue graphicsQueue);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
	VkDevice GetLogicalDevice() const { return logicalDevice; }
	VulkanSwapChain* GetSwapChain() const { return swapChain.get(); }
	VulkanDepthBuffer* GetDepthBuffer() const { return depthBuffer.get(); }
	VkCommandPool GetCommandPool() const { return commandPool; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue; }
	VkQueue GetPresentQueue() const { return presentQueue; }

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	void CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	void CreateCommandPool();

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkCommandPool commandPool;

	std::unique_ptr<VulkanSwapChain> swapChain;
	std::unique_ptr<VulkanDepthBuffer> depthBuffer;
};

#endif // !VULKAN_DEVICE_H