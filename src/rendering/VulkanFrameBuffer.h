#ifndef VULKAN_FRAMEBUFFER_H
#define VULKAN_FRAMEBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanDepthBuffer.h"

class VulkanFramebuffer
{
public:
	VulkanFramebuffer(VulkanDevice& device, VulkanSwapChain& swapChain, VulkanRenderPass& renderPass, VulkanDepthBuffer& depthBuffer);
	~VulkanFramebuffer();

	VkFramebuffer GetFramebuffer(int index) const { return framebuffers[index]; }

private:
	void CreateFramebuffers();

	VulkanDevice& device;
	VulkanSwapChain& swapChain;
	VulkanRenderPass& renderPass;
	VulkanDepthBuffer& depthBuffer;

	std::vector<VkFramebuffer> framebuffers;
};

#endif // !VULKAN_FRAMEBUFFER_H
