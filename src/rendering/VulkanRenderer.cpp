#include "VulkanRenderer.h"
#include <iostream>
#include <stdexcept>
#include "Vertex.h"

static inline glm::mat4 MakeTerrainModel(float yDown, float uniformScale = 1.0f)
{
	glm::mat4 m(1.0f);
	m = glm::translate(m, glm::vec3(0.0f, -yDown, 0.0f));
	m = glm::scale(m, glm::vec3(uniformScale, uniformScale, uniformScale));
	return m;
};

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}

void VulkanRenderer::Init(GLFWwindow* window)
{
	windowHandle = window;

	CreateInstance();
	CreateSurface(window);

	device = std::make_unique<VulkanDevice>(vulkanInstance, surface);

	Material::InitTextureStaging(*device, 16ull * 1024ull * 1024ull); // 16 MB staging buffer for textures
	descriptorPools.Init(device->GetLogicalDevice());

	scene = std::make_unique<Scene>();
	// TEMP: Cube for testing/reference
	ModelLoader::LoadModel(MODEL_PATH, *device, meshBatch, *scene, descriptorPools.GetMaterialPool());
	auto m = glm::scale(glm::mat4(1.0), glm::vec3(0.01f));
	scene->GetInstances().at(0).transform = m; // Scale down the cube

	renderPass = std::make_unique<VulkanRenderPass>(*device, *device->GetSwapChain());
	framebuffer = std::make_unique<VulkanFramebuffer>(*device, *device->GetSwapChain(), *renderPass, *device->GetDepthBuffer());
	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass, Material::GetDescriptorSetLayoutStatic(*device));

	mvpBuffer = std::make_unique<UniformBuffer<UniformBufferObject>>(device->GetLogicalDevice(), device->GetPhysicalDevice());

	// ---------- Descriptor Pool and Set for MVP (Set 0) ----------
	VkDescriptorPoolSize uboPoolSize{};
	uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboPoolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo uboPoolInfo{};
	uboPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	uboPoolInfo.poolSizeCount = 1;
	uboPoolInfo.pPoolSizes = &uboPoolSize;
	uboPoolInfo.maxSets = 1;
	uboPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device->GetLogicalDevice(), &uboPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool for UBO!");
	}

	// Allocate UBO descriptor set
	VkDescriptorSetLayout uboLayout = graphicsPipeline->GetUniformBufferLayout();

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &uboLayout;

	if (vkAllocateDescriptorSets(device->GetLogicalDevice(), &allocInfo, &mvpDescriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate UBO descriptor set!");
	}

	// Write UBO descriptor
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = mvpBuffer->GetBuffer();
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = mvpDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device->GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);

	// ---- Create Command Buffer (binds mvpDescriptorSet and per-instance material descriptor sets later) ----
	commandBuffer = std::make_unique<VulkanCommandBuffer>(
		*device,
		*device->GetSwapChain(),
		*renderPass,
		*framebuffer,
		*graphicsPipeline,
		mvpDescriptorSet, // <- This is only Set 0
		scene->GetInstances().at(0).material->GetDescriptorSet() // <- This is Set 1
	);

	// ---- initialize worldgen single chunk ----

	worldgen = std::make_unique<WorldgenSystem>(*device, meshBatch);
	worldgen->settings.vertsPerSide = 129; // 128 cells, 129 vertices
	worldgen->settings.cellSize = 1.0f; // 1 meter per cell
	worldgen->settings.skirtHeight = 0.5f; // 0.5 meter skirt height
	worldgen->lod.maxLOD = 3; // Max LOD level (0 = highest detail)
	worldgen->lod.fullDetailRings = 1; // Number of rings at full detail (center chunk + 1 ring)
	worldgen->InitSingleChunk(1337u); // Initialize with a fixed seed
	worldgen->StartWorkers(); // Start the worker thread for async generation

	// ---------- Sync objects ----------
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

	// ---------- Camera and Input ----------
	float aspect = (float)device->GetSwapChain()->GetSwapChainExtent().width / (float)device->GetSwapChain()->GetSwapChainExtent().height;
	camera = std::make_unique<Camera>(45.0f, aspect, 0.1f, 10000.0f);

	inputHandler = std::make_unique<InputHandler>(window, *camera);
	inputHandler->SetOnReloadShaders([this]() { this->ReloadShaders(); });
}

void VulkanRenderer::Cleanup()
{
	if (device)
	{
		if (worldgen)
		{
			worldgen->Shutdown(); // Stop the worker thread
		}

		// Ensure device isn't doing any work
		vkDeviceWaitIdle(device->GetLogicalDevice());

		// Clear scene objects and destroy uploaded mesh buffers
		if (scene)
		{
			scene->Clear();
			scene.reset();
		}

		ModelCacheManager::materialCache.clear();
		meshBatch.Destroy(device->GetLogicalDevice());

		// Destroy material descriptor pool
		Material::DestroySamplerCache(*device);
		Material::DestroyDescriptorSetLayoutStatic(*device);
		Material::DestroyTextureStaging(*device);
		descriptorPools.Destroy();

		// Destroy the descriptor pool used for the MVP uniform buffer
		if (descriptorPool != VK_NULL_HANDLE)
		{
			if (mvpDescriptorSet != VK_NULL_HANDLE)
			{
				vkFreeDescriptorSets(device->GetLogicalDevice(), descriptorPool, 1, &mvpDescriptorSet);
				mvpDescriptorSet = VK_NULL_HANDLE;
			}

			vkDestroyDescriptorPool(device->GetLogicalDevice(), descriptorPool, nullptr);
			descriptorPool = VK_NULL_HANDLE;
		}

		// Destroy command buffer, pipeline, etc.
		commandBuffer.reset();
		mvpBuffer.reset();
		graphicsPipeline.reset();
		framebuffer.reset();
		renderPass.reset();

		// Destroy sync objects
		if (imageAvailableSemaphore)
		{
			vkDestroySemaphore(device->GetLogicalDevice(), imageAvailableSemaphore, nullptr);
			imageAvailableSemaphore = VK_NULL_HANDLE;
		}

		if (renderFinishedSemaphore)
		{
			vkDestroySemaphore(device->GetLogicalDevice(), renderFinishedSemaphore, nullptr);
			renderFinishedSemaphore = VK_NULL_HANDLE;
		}

		if (inFlightFence)
		{
			vkDestroyFence(device->GetLogicalDevice(), inFlightFence, nullptr);
			inFlightFence = VK_NULL_HANDLE;
		}

		// Destroy the Vulkan device
		device.reset();
	}

	// Destroy the surface (used with swapchain)
	if (surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		surface = VK_NULL_HANDLE;
	}

	// Finally destroy the Vulkan instance
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

void VulkanRenderer::RebuildCommandBuffer() {
	if (!scene || scene->GetInstances().empty())
		return;

	commandBuffer.reset();
	commandBuffer = std::make_unique<VulkanCommandBuffer>(
		*device,
		*device->GetSwapChain(),
		*renderPass,
		*framebuffer,
		*graphicsPipeline,
		mvpDescriptorSet,
		scene->GetInstances().at(0).material->GetDescriptorSet() // assumes set 1 for materials
	);

	commandBufferDirty = false;
}

void VulkanRenderer::DrawFrame()
{
	vkWaitForFences(device->GetLogicalDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device->GetLogicalDevice(), 1, &inFlightFence);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		device->GetLogicalDevice(),
		device->GetSwapChain()->GetSwapChain(),
		UINT64_MAX,
		imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&imageIndex);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	UpdateUniformBuffer();
	commandBuffer->BeginRecording(imageIndex);

	// --- Draw regular scene instances (textured) ---
	for (const auto& instance : scene->GetInstances())
	{
		VkDescriptorSet sets[] = {
			mvpDescriptorSet,                    // set=0 (UBO)
			instance.material->GetDescriptorSet()// set=1 (material)
		};

		vkCmdBindDescriptorSets(
			commandBuffer->GetCommandBuffer(imageIndex),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipeline->GetPipelineLayout(),
			0, 2, sets, 0, nullptr);

		commandBuffer->BindPushConstants(
			instance.transform,
			camera->GetViewMatrix(),
			camera->GetProjectionMatrix(),
			/*useTexture=*/1,
			/*baseColor=*/glm::vec3(1.0f) // ignored by textured path
		);

		instance.mesh->Bind(commandBuffer->GetCommandBuffer(imageIndex));
		instance.mesh->Draw(commandBuffer->GetCommandBuffer(imageIndex));
	}

	// -- Draw terrain (solid white, still lit) --
	if (worldgen)
	{
		worldgen->BuildVisibleSet(camera->GetViewMatrix(), camera->GetProjectionMatrix());

		VkDescriptorSet materialSet = !scene->GetInstances().empty()
			? scene->GetInstances().at(0).material->GetDescriptorSet()
			: VK_NULL_HANDLE;

		if (materialSet != VK_NULL_HANDLE) {
			VkDescriptorSet sets[] = { mvpDescriptorSet, materialSet };
			vkCmdBindDescriptorSets(
				commandBuffer->GetCommandBuffer(imageIndex),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				graphicsPipeline->GetPipelineLayout(),
				0, 2, sets, 0, nullptr);
		}

		glm::mat4 model(1.0f);
		commandBuffer->BindPushConstants(
			model,
			camera->GetViewMatrix(),
			camera->GetProjectionMatrix(),
			/*useTexture=*/0,
			/*baseColor=*/glm::vec3(0.85f)
		);

		for (Mesh* m : worldgen->VisibleMeshes()) {
			if (!m) continue;
			m->Bind(commandBuffer->GetCommandBuffer(imageIndex));
			m->Draw(commandBuffer->GetCommandBuffer(imageIndex));
		}
	}

	commandBuffer->EndRecording(imageIndex);

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	VkCommandBuffer cmdBuffer = commandBuffer->GetCommandBuffer(imageIndex);
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (device->SubmitGraphicsLocked(&submitInfo, 1, inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChain = device->GetSwapChain()->GetSwapChain();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &imageIndex;

	result = device->PresentLocked(&presentInfo);
	if (result != VK_SUCCESS) {
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

	for (auto& instance : scene->GetInstances()) {
		if (instance.material) {
			instance.material->RecreateDescriptorSetLayout(device->GetLogicalDevice());
		}
	}

	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass, Material::GetDescriptorSetLayoutStatic(*device));
	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, mvpDescriptorSet, scene->GetInstances().at(0).material->GetDescriptorSet());

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

	graphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*device, *device->GetSwapChain(), *renderPass, Material::GetDescriptorSetLayoutStatic(*device));
	commandBuffer = std::make_unique<VulkanCommandBuffer>(*device, *device->GetSwapChain(), *renderPass, *framebuffer, *graphicsPipeline, mvpDescriptorSet, scene->GetInstances().at(0).material->GetDescriptorSet());

	std::cout << "[INFO] Shaders reloaded" << std::endl;
}

void VulkanRenderer::Update(float deltaTime)
{
	if (inputHandler)
	{
		inputHandler->Update(deltaTime);
	}

	if (worldgen && camera)
	{
		worldgen->Update(camera->GetPosition());
		worldgen->PumpUploads(3);
	}

	if (auto result = asyncLoader.GetResult())
	{
		if (result->success)
		{
			std::cout << "[VulkanRenderer] Model loaded async\n";

			vkDeviceWaitIdle(device->GetLogicalDevice());

			scene->Clear();
			meshBatch.Destroy(device->GetLogicalDevice());

			meshBatch = std::move(result->meshBatch);
			scene = std::move(result->scene);

			scene->SetDevice(device.get());
			scene->SetMeshBatch(&meshBatch);
			scene->Upload(*device);

			RebuildCommandBuffer();
		}
		else
		{
			std::cerr << "[VulkanRenderer] Failed to load model async\n";
		}
	}

	UpdateFPS(deltaTime);
}


void VulkanRenderer::UpdateUniformBuffer() {
	UniformBufferObject ubo{};

	ubo.model = glm::mat4(1.0f);
	//ubo.model = glm::scale(ubo.model, glm::vec3(0.05f, -0.05f, 0.05f));

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

void VulkanRenderer::LoadModelAsync(const std::string& path)
{
	asyncLoader.RequestLoad(path, *device, descriptorPools.GetMaterialPool());
}

void VulkanRenderer::UpdateFPS(float dt)
{
	fpsFrames++;
	fpsAccum += dt;

	if (fpsAccum >= 0.5) { // update title twice per second
		fpsValue = static_cast<float>(fpsFrames / fpsAccum);
		msPerFrame = (fpsValue > 0.0001f) ? (1000.0f / fpsValue) : 0.0f;

		if (windowHandle) {
			char title[160];
			std::snprintf(title, sizeof(title),
				"YorEngine  |  %.1f FPS (%.2f ms)", fpsValue, msPerFrame);
			glfwSetWindowTitle(windowHandle, title);
		}

		fpsFrames = 0;
		fpsAccum = 0.0;
	}
}