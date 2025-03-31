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
#include "UniformBuffer.h"
#include "UniformBufferObject.h"
#include "ModelLoader.h"
#include "Scene.h"
#include "MeshBatch.h"

#include "../core/Camera.h"

#include "../input/InputHandler.h"

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
	void Update(float deltaTime);
	Camera* GetCamera() { return camera.get(); }
	Scene& GetScene() { return *scene; }
	VulkanDevice* GetDevice() { return device.get(); }
	MeshBatch& GetMeshBatch() { return meshBatch; }

private:
	void CreateInstance();
	void CreateSurface(GLFWwindow* window);
	std::vector<const char*> GetRequiredExtensions();

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

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
	std::unique_ptr<UniformBuffer<UniformBufferObject>> mvpBuffer;
	std::unique_ptr<Camera> camera;
	std::unique_ptr<InputHandler> inputHandler;
	std::unique_ptr<Scene> scene;

	MeshBatch meshBatch;
};

#endif // !VULKAN_RENDERER_H
