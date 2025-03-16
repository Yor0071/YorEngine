#include "VulkanDepthBuffer.h"
#include "VulkanDevice.h"

#include <stdexcept>
#include <vector>

VulkanDepthBuffer::VulkanDepthBuffer(VulkanDevice& device, VkExtent2D extent)
	: device(device), extent(extent)
{
	CreateDepthResources();
}

VulkanDepthBuffer::~VulkanDepthBuffer()
{
	if (depthImageView != VK_NULL_HANDLE)
		vkDestroyImageView(device.GetLogicalDevice(), depthImageView, nullptr);

	if (depthImage != VK_NULL_HANDLE)
		vkDestroyImage(device.GetLogicalDevice(), depthImage, nullptr);

	if (depthImageMemory != VK_NULL_HANDLE)
		vkFreeMemory(device.GetLogicalDevice(), depthImageMemory, nullptr);
}

void VulkanDepthBuffer::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = depthFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device.GetLogicalDevice(), &imageInfo, nullptr, &depthImage) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create depth image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device.GetLogicalDevice(), depthImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device.GetLogicalDevice(), &allocInfo, nullptr, &depthImageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate depth image memory");
	}

	vkBindImageMemory(device.GetLogicalDevice(), depthImage, depthImageMemory, 0);

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device.GetLogicalDevice(), &viewInfo, nullptr, &depthImageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create depth image view");
	}
}

VkFormat VulkanDepthBuffer::FindDepthFormat()
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat VulkanDepthBuffer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format");
}