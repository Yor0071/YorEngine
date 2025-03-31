#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device,
                                         VulkanSwapChain& swapChain,
                                         VulkanRenderPass& renderPass,
                                         VulkanFramebuffer& framebuffer,
                                         VulkanGraphicsPipeline& graphicsPipeline,
                                         VkDescriptorSet descriptorSet)
                                         : device(device),
                                         swapChain(swapChain),
                                         renderPass(renderPass),
                                         framebuffer(framebuffer),
                                         graphicsPipeline(graphicsPipeline),
	                                     descriptorSet(descriptorSet)
{
    CreateCommandBuffers();
}

VulkanCommandBuffer::~VulkanCommandBuffer() 
{
	vkFreeCommandBuffers(device.GetLogicalDevice(), device.GetCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	std::cout << "Command buffers destroyed" << std::endl;
}

void VulkanCommandBuffer::CreateCommandBuffers()
{
	commandBuffers.resize(swapChain.GetSwapChainImageCount());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = device.GetCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device.GetLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}

	std::cout << "Command buffers allocated" << std::endl;
}

void VulkanCommandBuffer::BeginRecording(uint32_t imageIndex)
{
	currentImageIndex = imageIndex;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass.GetRenderPass();
	renderPassInfo.framebuffer = framebuffer.GetFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChain.GetSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { { 0.1f, 0.1f, 0.3f, 1.0f } };  // Color
	clearValues[1].depthStencil = { 1.0f, 0 };              // Depth

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipeline());

	vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
}

void VulkanCommandBuffer::EndRecording(uint32_t imageIndex)
{
	vkCmdEndRenderPass(commandBuffers[imageIndex]);

	if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

void VulkanCommandBuffer::BindPushConstants(const glm::mat4& modelMatrix)
{
	vkCmdPushConstants(commandBuffers[currentImageIndex], graphicsPipeline.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);
}

VkCommandBuffer VulkanCommandBuffer::GetCommandBuffer(uint32_t imageIndex) const
{
	return commandBuffers[imageIndex];
}
	