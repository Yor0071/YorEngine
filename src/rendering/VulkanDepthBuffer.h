#ifndef VULKAN_DEPTH_BUFFER_H
#define VULKAN_DEPTH_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;

class VulkanDepthBuffer
{
public:
	VulkanDepthBuffer(VulkanDevice& device, VkExtent2D extent);
	~VulkanDepthBuffer();

	VkImageView GetDepthImageView() { return depthImageView; }

private:
	void CreateDepthResources();

	VkFormat FindDepthFormat();
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	VulkanDevice& device;
	VkExtent2D extent;
	VkImage depthImage = VK_NULL_HANDLE;
	VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
	VkImageView depthImageView = VK_NULL_HANDLE;
};

#endif // !VULKAN_DEPTH_BUFFER_H