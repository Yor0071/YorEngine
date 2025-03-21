#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device,
                                         VulkanSwapChain& swapChain,
                                         VulkanRenderPass& renderPass,
                                         VulkanFramebuffer& framebuffer,
                                         VulkanGraphicsPipeline& graphicsPipeline,
                                         VertexBuffer& vertexBuffer,
	                                     IndexBuffer& indexBuffer,
                                         VkDescriptorSet descriptorSet)
                                         : device(device),
                                         swapChain(swapChain),
                                         renderPass(renderPass),
                                         framebuffer(framebuffer),
                                         graphicsPipeline(graphicsPipeline),
                                         vertexBuffer(vertexBuffer),
	                                     indexBuffer(indexBuffer),
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

void VulkanCommandBuffer::RecordCommandBuffer(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass.GetRenderPass();
    renderPassInfo.framebuffer = framebuffer.GetFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChain.GetSwapChainExtent();

    VkClearValue clearValues[2]{};
    clearValues[0].color = { {0.2f, 0.3f, 0.8f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipeline());

	VkBuffer vertexBuffers[] = { vertexBuffer.GetBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffers[imageIndex], indexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(commandBuffers[imageIndex], indexBuffer.GetIndexCount(), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffers[imageIndex]);

    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}
	