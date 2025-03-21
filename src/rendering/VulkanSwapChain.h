#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>
#include "QueueFamilyIndices.h"

struct QueueFamilyIndices;

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapChain
{
public:
	VulkanSwapChain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface, QueueFamilyIndices queueFamilyIndices);
	~VulkanSwapChain();

	VkSwapchainKHR GetSwapChain() { return swapChain; }
	const std::vector<VkImage>& GetSwapChainImages() { return swapChainImages; }
	const std::vector<VkImageView>& GetSwapChainImageViews() { return swapChainImageViews; }
	VkFormat GetSwapChainImageFormat() const { return swapChainImageFormat; }
	VkExtent2D GetSwapChainExtent() const { return swapChainExtent; }
	uint32_t GetSwapChainImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }

private:
	void CreateSwapChain();
	void CreateImageViews();

	QueueFamilyIndices queueFamilyIndices;

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkDevice logicalDevice;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
};


#endif // !VULKAN_SWAPCHAIN_H