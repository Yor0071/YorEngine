#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool IsComplete() const
	{
		return graphicsFamily != -1 && presentFamily != -1;
	}
};

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
};

#endif // !VULKAN_DEVICE_H