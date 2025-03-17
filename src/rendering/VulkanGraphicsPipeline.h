#ifndef VULKAN_GRAPHICS_PIPELINE_H
#define VULKAN_GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"

class VulkanGraphicsPipeline
{
public:
	VulkanGraphicsPipeline(VulkanDevice& device, VulkanSwapChain& swapChain, VulkanRenderPass& renderPass);
	~VulkanGraphicsPipeline();

	VkPipeline GetPipeline() const { return graphicsPipeline; }
	VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

private:
	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	std::vector<char> ReadFile(const std::string& filename);

	VulkanDevice& device;
	VulkanSwapChain& swapChain;
	VulkanRenderPass& renderPass;

	VkPipeline graphicsPipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

#endif // !VULKAN_GRAPHICS_PIPELINE_H
