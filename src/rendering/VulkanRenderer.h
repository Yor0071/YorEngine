#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	void Init(GLFWwindow* window);
	void Cleanup();

private:
	void CreateInstance();
	std::vector<const char*> GetRequiredExtensions();

	VkInstance vulkanInstance;
};

#endif // !VULKAN_RENDERER_H
