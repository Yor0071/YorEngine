#ifndef MATERIAL_H
#define MATERIAL_H

#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <cmath>

#include "VulkanDevice.h"

class VulkanDevice;

class Material
{
public:
	Material(VulkanDevice& device, const std::string& texturePath, VkDescriptorPool sharedPool);
	~Material();

	VkDescriptorSet GetDescriptorSet() const { return descriptorSet; }
	VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout; }
	static VkDescriptorSetLayout GetDescriptorSetLayoutStatic(VulkanDevice& device);
	const std::string& GetTexturePath() const { return texturePath; }
	void RecreateDescriptorSetLayout(VkDevice device);
	static void DestroyDescriptorSetLayoutStatic(VulkanDevice& device);
	static void DestroySamplerCache(VulkanDevice& device);

	static void InitTextureStaging(VulkanDevice& device, VkDeviceSize sizeBytes);
	static void DestroyTextureStaging(VulkanDevice& device);
private:
	void LoadTexture(const std::string& path);
	void CreateTextureImage(const std::string& path);
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateDescriptorSetLayout();
	void AllocateAndWriteDescriptorSet();
	static void GenerateMipmapsNow(VulkanDevice& device, VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	std::string texturePath;
	VulkanDevice& device;

	VkImage textureImage = VK_NULL_HANDLE;
	VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	//VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorPool externalDescriptorPool = VK_NULL_HANDLE; // Reference to the shared pool

	uint32_t textureMipLevels = 1;
};

#endif // !MATERIAL_H
