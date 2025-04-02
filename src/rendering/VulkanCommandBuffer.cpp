#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice& device,
										 VulkanSwapChain& swapChain,
										 VulkanRenderPass& renderPass,
										 VulkanFramebuffer& framebuffer,
										 VulkanGraphicsPipeline& graphicsPipeline,
										 VkDescriptorSet mvpSet,             // set = 0
										 VkDescriptorSet materialSet)		 // set = 1
										 : device(device),
										 swapChain(swapChain),
										 renderPass(renderPass),
										 framebuffer(framebuffer),
										 graphicsPipeline(graphicsPipeline),
										 mvpDescriptorSet(mvpSet),
										 materialDescriptorSet(materialSet)
{
    CreateCommandBuffers();
}

VulkanCommandBuffer::~VulkanCommandBuffer() 
{
	vkFreeCommandBuffers(device.GetLogicalDevice(), commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	std::cout << "Command buffers destroyed" << std::endl;
}

void VulkanCommandBuffer::CreateCommandBuffers()
{
	commandBuffers.resize(swapChain.GetSwapChainImageCount());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	// Use thread-local pool and store it
	commandPool = device.GetThreadCommandPool()->GetOrCreatePoolForCurrentThread();
	allocInfo.commandPool = commandPool;

	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device.GetLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}

	std::cout << "Command buffers allocated\n";
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

	VkDescriptorSet descriptorSets[] = { mvpDescriptorSet, materialDescriptorSet };
	vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetPipelineLayout(), 0, 2, descriptorSets, 0, nullptr);
}

void VulkanCommandBuffer::EndRecording(uint32_t imageIndex)
{
	vkCmdEndRenderPass(commandBuffers[imageIndex]);

	if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

void VulkanCommandBuffer::BindPushConstants(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj)
{
	struct PushConstants
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	} data;

	data.model = model;
	data.view = view;
	data.proj = proj;

	vkCmdPushConstants(
		commandBuffers[currentImageIndex],
		graphicsPipeline.GetPipelineLayout(),
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(PushConstants),
		&data);
}

VkCommandBuffer VulkanCommandBuffer::GetCommandBuffer(uint32_t imageIndex) const
{
	return commandBuffers[imageIndex];
}
	