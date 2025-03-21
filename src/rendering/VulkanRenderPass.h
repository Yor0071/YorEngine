#ifndef VULKAN_RENDER_PASS_H
#define VULKAN_RENDER_PASS_H

#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

class VulkanRenderPass
{
public:
	VulkanRenderPass(VulkanDevice& device, VulkanSwapChain& swapChain);
	~VulkanRenderPass();

	VkRenderPass GetRenderPass() const { return renderPass; }

private:
	void CreateRenderPass();

	VulkanDevice& device;
	VulkanSwapChain& swapChain;

	VkRenderPass renderPass = VK_NULL_HANDLE;
};

#endif // !VULKAN_RENDER_PASS_H