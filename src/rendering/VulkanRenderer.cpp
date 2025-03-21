#include "VulkanRenderer.h"
#include <iostream>
#include <stdexcept>
#include "Vertex.h"

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}

// TEMPORARY
const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

void VulkanRenderer::Init(GLFWwindow* window)
{
	CreateInstance();
	CreateSurface(window);

	device = std::make_unique<VulkanDevice>(vulkanInstance, surface);
	renderPass = std::make_unique<VulkanRenderPass>(*device, *device->GetSwapChain());
	framebuffer = std::make_unique<VulkanFramebuffer>(*device, *device->GetSwapChain(), *renderPass, *device->GetDepthBuffer());
	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass);
	vertexBuffer = std::make_unique<VertexBuffer>(*device, vertices.data(), sizeof(vertices[0]) * vertices.size());
	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, *vertexBuffer);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device->GetLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device->GetLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create semaphores!");
	}

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateFence(device->GetLogicalDevice(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create fence!");
	}
}

void VulkanRenderer::Cleanup()
{
	if (device) {
		vkDeviceWaitIdle(device->GetLogicalDevice());

		if (imageAvailableSemaphore) {
			vkDestroySemaphore(device->GetLogicalDevice(), imageAvailableSemaphore, nullptr);
			imageAvailableSemaphore = VK_NULL_HANDLE;
		}

		if (renderFinishedSemaphore) {
			vkDestroySemaphore(device->GetLogicalDevice(), renderFinishedSemaphore, nullptr);
			renderFinishedSemaphore = VK_NULL_HANDLE;
		}

		if (inFlightFence) {
			vkDestroyFence(device->GetLogicalDevice(), inFlightFence, nullptr);
			inFlightFence = VK_NULL_HANDLE;
		}
	}

	vertexBuffer.reset();
	commandBuffer.reset();
	graphicsPipeline.reset();
	framebuffer.reset();
	renderPass.reset();
	device.reset();

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		surface = VK_NULL_HANDLE;
	}

	if (vulkanInstance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(vulkanInstance, nullptr);
		vulkanInstance = VK_NULL_HANDLE;
	}
}

void VulkanRenderer::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "YorEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "YorEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}

	std::cout << "Vulkan instance created" << std::endl;
}

void VulkanRenderer::CreateSurface(GLFWwindow* window)
{
	if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}

	std::cout << "Window surface created" << std::endl;
}

void VulkanRenderer::DrawFrame()
{
	vkWaitForFences(device->GetLogicalDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device->GetLogicalDevice(), 1, &inFlightFence);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device->GetLogicalDevice(), device->GetSwapChain()->GetSwapChain(),
		UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	commandBuffer->RecordCommandBuffer(imageIndex);

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	VkCommandBuffer cmdBuffer = commandBuffer->GetCommandBuffer(imageIndex);
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	VkSwapchainKHR swapChain = device->GetSwapChain()->GetSwapChain();
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image!");
	}
}

void VulkanRenderer::ReCreateSwapChain(GLFWwindow* window)
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device->GetLogicalDevice());

	framebuffer.reset();
	graphicsPipeline.reset();
	renderPass.reset();
	commandBuffer.reset();

	device->RecreateSwapChain();
	renderPass = std::make_unique<VulkanRenderPass>(*device, *device->GetSwapChain());
	framebuffer = std::make_unique<VulkanFramebuffer>(*device, *device->GetSwapChain(), *renderPass, *device->GetDepthBuffer());
	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass);
	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, *vertexBuffer);
}

void VulkanRenderer::ReloadShaders()
{
	vkDeviceWaitIdle(device->GetLogicalDevice());

	std::cout << "[INFO] Reloading shaders..." << std::endl;

	graphicsPipeline.reset();
	commandBuffer.reset();

	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass);
	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, *vertexBuffer);

	std::cout << "[INFO] Shaders reloaded" << std::endl;
}

std::vector<const char*> VulkanRenderer::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}