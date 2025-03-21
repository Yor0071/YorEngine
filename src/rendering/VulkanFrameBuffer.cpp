#include "VulkanFrameBuffer.h"
#include <stdexcept>
#include <iostream>

VulkanFramebuffer::VulkanFramebuffer(VulkanDevice& device, VulkanSwapChain& swapChain, VulkanRenderPass& renderPass, VulkanDepthBuffer& depthBuffer)
	: device(device), swapChain(swapChain), renderPass(renderPass), depthBuffer(depthBuffer)
{
	CreateFramebuffers();
}

VulkanFramebuffer::~VulkanFramebuffer()
{
	for (auto framebuffer : framebuffers)
	{
		if (framebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(device.GetLogicalDevice(), framebuffer, nullptr);
		}
		std::cout << "Framebuffer destroyed" << std::endl;
	}

	framebuffers.clear();
	std::cout << "Framebuffers cleared" << std::endl;
}

void VulkanFramebuffer::CreateFramebuffers()
{
	const auto& swapChainImageViews = swapChain.GetSwapChainImageViews();
	framebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			depthBuffer.GetDepthImageView()
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass.GetRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChain.GetSwapChainExtent().width;
		framebufferInfo.height = swapChain.GetSwapChainExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device.GetLogicalDevice(), &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer");
		}
	}

	std::cout << "Framebuffers created" << std::endl;
}