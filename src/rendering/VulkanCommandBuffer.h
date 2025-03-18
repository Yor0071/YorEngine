#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <iostream>

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"

class VulkanCommandBuffer
{
public:
	VulkanCommandBuffer(VulkanDevice& device, VulkanSwapChain& swapChain, VulkanRenderPass& renderPass, VulkanFramebuffer& framebuffer);
	~VulkanCommandBuffer();

	void RecordCommandBuffer(uint32_t imageIndex);
	VkCommandBuffer GetCommandBuffer(uint32_t index) const { return commandBuffers[index]; }

private:
	void CreateCommandBuffers();

	VulkanDevice& device;
	VulkanSwapChain& swapChain;
	VulkanRenderPass& renderPass;
	VulkanFramebuffer& framebuffer;

	std::vector<VkCommandBuffer> commandBuffers;
};

#endif // !VULKAN_COMMAND_BUFFER_H
