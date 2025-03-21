#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanCommandBuffer.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	void Init(GLFWwindow* window);
	void Cleanup();
	void DrawFrame();
	void ReCreateSwapChain(GLFWwindow* window);
	void ReloadShaders();

private:
	void CreateInstance();
	void CreateSurface(GLFWwindow* window);
	std::vector<const char*> GetRequiredExtensions();

	VkInstance vulkanInstance;
	VkSurfaceKHR surface;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;
	
	std::unique_ptr<VulkanDevice> device;
	std::unique_ptr<VulkanRenderPass> renderPass;
	std::unique_ptr<VulkanFramebuffer> framebuffer;
	std::unique_ptr<VulkanGraphicsPipeline> graphicsPipeline;
	std::unique_ptr<VulkanCommandBuffer> commandBuffer;
};

#endif // !VULKAN_RENDERER_H
