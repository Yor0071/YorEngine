#include "VulkanRenderer.h"
#include <iostream>
#include <stdexcept>

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}

void VulkanRenderer::Init(GLFWwindow* window)
{
	CreateInstance();
	CreateSurface(window);

	device = std::make_unique<VulkanDevice>(vulkanInstance, surface);

	renderPass = std::make_unique<VulkanRenderPass>(*device, *device->GetSwapChain());

	framebuffer = std::make_unique<VulkanFramebuffer>(*device, *device->GetSwapChain(), *renderPass, *device->GetDepthBuffer());

	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass);
}

void VulkanRenderer::Cleanup()
{
	if (graphicsPipeline)
	{
		graphicsPipeline.reset();
	}

	if (framebuffer)
	{
		framebuffer.reset();
	}

	if (renderPass)
	{
		renderPass.reset();
	}

	if (device)
	{
		device.reset();
	}

	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		std::cout << "Window surface destroyed" << std::endl;
		surface = VK_NULL_HANDLE;
	}

	vkDestroyInstance(vulkanInstance, nullptr);
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

std::vector<const char*> VulkanRenderer::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}