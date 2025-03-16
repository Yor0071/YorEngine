#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <memory>
#include "QueueFamilyIndices.h"

class VulkanSwapChain;

class VulkanDevice
{
public:
	VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
	~VulkanDevice();

	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
	VkDevice GetLogicalDevice() const { return logicalDevice; }

private:
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	void CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	std::unique_ptr<VulkanSwapChain> swapChain;
};

#endif // !VULKAN_DEVICE_H