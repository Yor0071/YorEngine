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
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VulkanGraphicsPipeline.h"

class VulkanCommandBuffer
{
public:
	VulkanCommandBuffer(VulkanDevice& device,
		VulkanSwapChain& swapChain,
		VulkanRenderPass& renderPass,
		VulkanFramebuffer& framebuffer,
		VulkanGraphicsPipeline& graphicsPipeline,
		VertexBuffer& vertexBuffer,
		IndexBuffer& indexBuffer,
		VkDescriptorSet descriptorSet);
	~VulkanCommandBuffer();

	void RecordCommandBuffer(uint32_t imageIndex);
	VkCommandBuffer GetCommandBuffer(uint32_t index) const { return commandBuffers[index]; }

private:
	void CreateCommandBuffers();

	VulkanDevice& device;
	VulkanSwapChain& swapChain;
	VulkanRenderPass& renderPass;
	VulkanFramebuffer& framebuffer;
	VulkanGraphicsPipeline& graphicsPipeline;
	VertexBuffer& vertexBuffer;
	IndexBuffer& indexBuffer;
	VkDescriptorSet descriptorSet;

	std::vector<VkCommandBuffer> commandBuffers;
};

#endif // !VULKAN_COMMAND_BUFFER_H
