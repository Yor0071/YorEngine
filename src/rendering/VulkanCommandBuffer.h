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
						VkDescriptorSet mvpSet,             // set = 0
						VkDescriptorSet materialSet);		// set = 1
	~VulkanCommandBuffer();

	void BeginRecording(uint32_t imageIndex);
	void EndRecording(uint32_t imageIndex);

	void BindPushConstants(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj);

	VkCommandBuffer GetCommandBuffer(uint32_t imageIndex) const;

private:
	void CreateCommandBuffers();

	VulkanDevice& device;
	VulkanSwapChain& swapChain;
	VulkanRenderPass& renderPass;
	VulkanFramebuffer& framebuffer;
	VulkanGraphicsPipeline& graphicsPipeline;
	VkDescriptorSet mvpDescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSet materialDescriptorSet = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;

	std::vector<VkCommandBuffer> commandBuffers;
	uint32_t currentImageIndex = 0;
};

#endif // !VULKAN_COMMAND_BUFFER_H
