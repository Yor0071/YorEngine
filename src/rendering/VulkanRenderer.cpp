#include "VulkanRenderer.h"
#include <iostream>
#include <stdexcept>
#include "Vertex.h"

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}

// ==================================TEMPORARY======================================================
const std::vector<Vertex> vertices = {
	// Front face
	{{-0.5f, -0.5f,  0.5f}, {1.f, 0.f, 0.f}}, // 0
	{{ 0.5f, -0.5f,  0.5f}, {0.f, 1.f, 0.f}}, // 1
	{{ 0.5f,  0.5f,  0.5f}, {0.f, 0.f, 1.f}}, // 2
	{{-0.5f,  0.5f,  0.5f}, {1.f, 1.f, 0.f}}, // 3

	// Back face
	{{-0.5f, -0.5f, -0.5f}, {1.f, 0.f, 1.f}}, // 4
	{{ 0.5f, -0.5f, -0.5f}, {0.f, 1.f, 1.f}}, // 5
	{{ 0.5f,  0.5f, -0.5f}, {1.f, 1.f, 1.f}}, // 6
	{{-0.5f,  0.5f, -0.5f}, {0.f, 0.f, 0.f}}, // 7
};

const std::vector<uint16_t> indices = {
	// Front face
	0, 1, 2,  2, 3, 0,
	// Right face
	1, 5, 6,  6, 2, 1,
	// Back face
	5, 4, 7,  7, 6, 5,
	// Left face
	4, 0, 3,  3, 7, 4,
	// Top face
	3, 2, 6,  6, 7, 3,
	// Bottom face
	4, 5, 1,  1, 0, 4
};

// =================================================================================================

void VulkanRenderer::Init(GLFWwindow* window)
{
	CreateInstance();
	CreateSurface(window);

	device = std::make_unique<VulkanDevice>(vulkanInstance, surface);
	renderPass = std::make_unique<VulkanRenderPass>(*device, *device->GetSwapChain());
	framebuffer = std::make_unique<VulkanFramebuffer>(*device, *device->GetSwapChain(), *renderPass, *device->GetDepthBuffer());
	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass);
	vertexBuffer = std::make_unique<VertexBuffer>(*device, vertices.data(), sizeof(vertices[0]) * vertices.size());
	indexBuffer = std::make_unique<IndexBuffer>(*device, indices.data(), sizeof(indices[0]) * indices.size(), static_cast<uint32_t>(indices.size()));
	mvpBuffer = std::make_unique<UniformBuffer<UniformBufferObject>>(device->GetLogicalDevice(), device->GetPhysicalDevice());

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(device->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	VkDescriptorSetLayout layouts[] = { graphicsPipeline->GetDescriptorSetLayout() };
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(device->GetLogicalDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = mvpBuffer->GetBuffer();
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device->GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);

	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, *vertexBuffer, *indexBuffer, descriptorSet);

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

	float aspect = (float)device->GetSwapChain()->GetSwapChainExtent().width / (float)device->GetSwapChain()->GetSwapChainExtent().height;
	camera = std::make_unique<Camera> (45.0f, aspect, 0.1f, 100.0f);
	inputHandler = std::make_unique<InputHandler>(window, *camera);
	inputHandler->SetOnReloadShaders([this]() { this->ReloadShaders(); });
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

	commandBuffer.reset();
	mvpBuffer.reset();
	indexBuffer.reset();
	vertexBuffer.reset();
	graphicsPipeline.reset();
	framebuffer.reset();
	renderPass.reset();

	if (descriptorPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(device->GetLogicalDevice(), descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
	}

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

	UpdateUniformBuffer();
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
	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, *vertexBuffer, *indexBuffer, descriptorSet);

	float newAspect = (float)device->GetSwapChain()->GetSwapChainExtent().width / (float)device->GetSwapChain()->GetSwapChainExtent().height;
	if (camera)
	{
		camera->SetAspectRatio(newAspect);
	}
}

void VulkanRenderer::ReloadShaders()
{
	vkDeviceWaitIdle(device->GetLogicalDevice());

	std::cout << "[INFO] Reloading shaders..." << std::endl;

	graphicsPipeline.reset();
	commandBuffer.reset();

	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass);
	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, *vertexBuffer, *indexBuffer, descriptorSet);

	std::cout << "[INFO] Shaders reloaded" << std::endl;
}

void VulkanRenderer::Update(float deltaTime)
{
	if (inputHandler)
	{
		inputHandler->Update(deltaTime);
	}
}

void VulkanRenderer::UpdateUniformBuffer() {
	UniformBufferObject ubo{};

	ubo.model = glm::rotate(glm::mat4(1.0f), static_cast<float>(glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));

	ubo.view = camera->GetViewMatrix();

	ubo.proj = camera->GetProjectionMatrix();

	ubo.proj[1][1] *= -1;

	mvpBuffer->Update(ubo);
}

std::vector<const char*> VulkanRenderer::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}