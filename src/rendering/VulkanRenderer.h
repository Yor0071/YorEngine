#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "VulkanDevice.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	void Init(GLFWwindow* window);
	void Cleanup();

private:
	void CreateInstance();
	void CreateSurface(GLFWwindow* window);
	std::vector<const char*> GetRequiredExtensions();

	VkInstance vulkanInstance;
	VkSurfaceKHR surface;
	VulkanDevice* device = nullptr;
};

#endif // !VULKAN_RENDERER_H
