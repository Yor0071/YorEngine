#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFrameBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanCommandBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "UniformBuffer.h"
#include "UniformBufferObject.h"

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
	void UpdateUniformBuffer();

private:
	void CreateInstance();
	void CreateSurface(GLFWwindow* window);
	std::vector<const char*> GetRequiredExtensions();

	VkInstance vulkanInstance;
	VkSurfaceKHR surface;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	
	std::unique_ptr<VulkanDevice> device;
	std::unique_ptr<VulkanRenderPass> renderPass;
	std::unique_ptr<VulkanFramebuffer> framebuffer;
	std::unique_ptr<VulkanGraphicsPipeline> graphicsPipeline;
	std::unique_ptr<VulkanCommandBuffer> commandBuffer;
	std::unique_ptr<VertexBuffer> vertexBuffer;
	std::unique_ptr<IndexBuffer> indexBuffer;
	std::unique_ptr<UniformBuffer<UniformBufferObject>> mvpBuffer;
};

#endif // !VULKAN_RENDERER_H
