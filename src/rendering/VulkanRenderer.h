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
#include "AsyncModelLoader.h"
#include "DescriptorPools.h"
#include "Material.h"
#include "Mesh.h"

#include "../core/Camera.h"

#include "../input/InputHandler.h"

#include "../utils/PerlinNoise.h"
#include "../ecs/TerrainComponent.h"

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
	void LoadModelAsync(const std::string& path);
	void MarkCommandBufferDirty() { commandBufferDirty = true; }
	void InitTerrain();
	Camera* GetCamera() { return camera.get(); }
	Scene& GetScene() { return *scene; }
	VulkanDevice* GetDevice() { return device.get(); }
	MeshBatch& GetMeshBatch() { return meshBatch; }
	VkDescriptorPool GetDescriptorPool() const { return descriptorPools.GetMaterialPool(); }

private:
	void CreateInstance();
	void CreateSurface(GLFWwindow* window);
	void RebuildCommandBuffer();
	std::vector<const char*> GetRequiredExtensions();

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkInstance vulkanInstance;
	VkSurfaceKHR surface;

	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence inFlightFence;

	DescriptorPools descriptorPools;

	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

	VkDescriptorSet mvpDescriptorSet = VK_NULL_HANDLE;
	
	std::unique_ptr<VulkanDevice> device;
	std::unique_ptr<VulkanRenderPass> renderPass;
	std::unique_ptr<VulkanFramebuffer> framebuffer;
	std::unique_ptr<VulkanGraphicsPipeline> graphicsPipeline;
	std::unique_ptr<VulkanCommandBuffer> commandBuffer;
	std::unique_ptr<UniformBuffer<UniformBufferObject>> mvpBuffer;
	std::unique_ptr<Camera> camera;
	std::unique_ptr<InputHandler> inputHandler;
	std::shared_ptr<Scene> scene;
	std::unique_ptr<Mesh> terrainMesh;

	MeshBatch meshBatch;
	AsyncModelLoader asyncLoader;

	bool commandBufferDirty = false;
};

#endif // !VULKAN_RENDERER_H
