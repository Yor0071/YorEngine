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
#include "VulkanGraphicsPipeline.h"

class VulkanCommandBuffer
{
public:
	VulkanCommandBuffer(VulkanDevice& device,
		VulkanSwapChain& swapChain,
		VulkanRenderPass& renderPass,
		VulkanFramebuffer& framebuffer,
		VulkanGraphicsPipeline& graphicsPipeline,
		VkDescriptorSet descriptorSet);
	~VulkanCommandBuffer();

	void BeginRecording(uint32_t imageIndex);
	void EndRecording(uint32_t imageIndex);

	void BindPushConstants(const glm::mat4& modelMatrix);

	VkCommandBuffer GetCommandBuffer(uint32_t imageIndex) const;

private:
	void CreateCommandBuffers();

	VulkanDevice& device;
	VulkanSwapChain& swapChain;
	VulkanRenderPass& renderPass;
	VulkanFramebuffer& framebuffer;
	VulkanGraphicsPipeline& graphicsPipeline;
	VkDescriptorSet descriptorSet;

	std::vector<VkCommandBuffer> commandBuffers;
	uint32_t currentImageIndex = 0;
};

#endif // !VULKAN_COMMAND_BUFFER_H
