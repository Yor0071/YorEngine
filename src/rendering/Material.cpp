#include "Material.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"

static std::mutex g_samplerCacheMutex;
static VkDescriptorSetLayout g_materialSetLayout = VK_NULL_HANDLE;

struct SamplerCacheKey
{
	float maxAnisotropy;
	VkFilter minFilter;
	VkFilter magFilter;
	VkSamplerAddressMode addressModeU;
	VkSamplerAddressMode addressModeV;
	VkSamplerAddressMode addressModeW;

	bool operator==(const SamplerCacheKey& other) const
	{
		return std::memcmp(this, &other, sizeof(SamplerCacheKey) == 0);
	}
};

struct SamplerCacheKeyHash
{
	size_t operator()(const SamplerCacheKey& k) const
	{
		return std::hash<float>()(k.maxAnisotropy) ^
			std::hash<int>()(k.minFilter) ^
			std::hash<int>()(k.magFilter) ^
			std::hash<int>()(k.addressModeU) ^
			std::hash<int>()(k.addressModeV) ^
			std::hash<int>()(k.addressModeW);
	}
};

static std::unordered_map<SamplerCacheKey, VkSampler, SamplerCacheKeyHash> g_samplerCache;

Material::Material(VulkanDevice& device, const std::string& texturePath, VkDescriptorPool sharedPool) 
	: device(device), texturePath(texturePath), externalDescriptorPool(sharedPool)
{
	assert(externalDescriptorPool != VK_NULL_HANDLE && "Material requires a valid shared descriptor pool");

	LoadTexture(texturePath);
	CreateTextureImageView();
	CreateTextureSampler();
	CreateDescriptorSetLayout();

	AllocateAndWriteDescriptorSet();
}

Material::~Material()
{
	VkDevice logicalDevice = device.GetLogicalDevice();

	if (descriptorSet != VK_NULL_HANDLE && externalDescriptorPool != VK_NULL_HANDLE)
	{
		vkFreeDescriptorSets(logicalDevice, externalDescriptorPool, 1, &descriptorSet);
		descriptorSet = VK_NULL_HANDLE;
	}

	if (textureImageView != VK_NULL_HANDLE)
		vkDestroyImageView(logicalDevice, textureImageView, nullptr);

	if (textureImage != VK_NULL_HANDLE)
		vkDestroyImage(logicalDevice, textureImage, nullptr);

	if (textureImageMemory != VK_NULL_HANDLE)
		vkFreeMemory(logicalDevice, textureImageMemory, nullptr);

	descriptorSetLayout = VK_NULL_HANDLE;
}

void Material::RecreateDescriptorSetLayout(VkDevice device)
{
	if (descriptorSet != VK_NULL_HANDLE && externalDescriptorPool != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(device, externalDescriptorPool, 1, &descriptorSet);
		descriptorSet = VK_NULL_HANDLE;
	}

	CreateDescriptorSetLayout();
	AllocateAndWriteDescriptorSet();
}

void Material::DestroyDescriptorSetLayoutStatic(VulkanDevice& device)
{
	if (g_materialSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(device.GetLogicalDevice(), g_materialSetLayout, nullptr);
		g_materialSetLayout = VK_NULL_HANDLE;
	}
}

void Material::DestroySamplerCache(VulkanDevice& device)
{
	std::lock_guard<std::mutex> lock(g_samplerCacheMutex);
	for (auto& kv : g_samplerCache)
		vkDestroySampler(device.GetLogicalDevice(), kv.second, nullptr);
	g_samplerCache.clear();
}

VkDescriptorSetLayout Material::GetDescriptorSetLayoutStatic(VulkanDevice& device)
{
	if (g_materialSetLayout == VK_NULL_HANDLE)
	{
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 0;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &samplerLayoutBinding;

		if (vkCreateDescriptorSetLayout(device.GetLogicalDevice(), &layoutInfo, nullptr, &g_materialSetLayout) != VK_SUCCESS)
			throw std::runtime_error("[Material] Failed to create static descriptor set layout.");
	}
	return g_materialSetLayout;
}

void Material::LoadTexture(const std::string& path)
{
	CreateTextureImage(path);
}

void Material::CreateTextureImage(const std::string& path)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		std::cerr << "[stb_image] Failed to load: " << path << "\nReason: " << stbi_failure_reason() << "\n";
	}

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
		throw std::runtime_error("[Material] Failed to load texture image.");

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	device.CreateBuffer(imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device.GetLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device.GetLogicalDevice(), stagingBufferMemory);

	stbi_image_free(pixels);

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(texWidth);
	imageInfo.extent.height = static_cast<uint32_t>(texHeight);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device.GetLogicalDevice(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to create texture image.");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device.GetLogicalDevice(), textureImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device.GetLogicalDevice(), &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to allocate image memory.");

	vkBindImageMemory(device.GetLogicalDevice(), textureImage, textureImageMemory, 0);

	device.CopyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	vkDestroyBuffer(device.GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(device.GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void Material::CreateTextureImageView()
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device.GetLogicalDevice(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to create texture image view.");
}

void Material::CreateTextureSampler()
{
	SamplerCacheKey key{};
	key.maxAnisotropy = 16.0f;
	key.minFilter = VK_FILTER_LINEAR;
	key.magFilter = VK_FILTER_LINEAR;
	key.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	key.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	key.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	// Fast path: try reusing an existing sampler from the cache
	{
		std::lock_guard<std::mutex> lock(g_samplerCacheMutex);
		auto it = g_samplerCache.find(key);
		if (it != g_samplerCache.end()) {
			textureSampler = it->second;
			return;
		}
	}

	// Slow path: create a new sampler
	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(), &props);

	VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	info.magFilter = key.magFilter;
	info.minFilter = key.minFilter;
	info.addressModeU = key.addressModeU;
	info.addressModeV = key.addressModeV;
	info.addressModeW = key.addressModeW;
	info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	info.anisotropyEnable = VK_TRUE;
	info.maxAnisotropy = std::min(key.maxAnisotropy, props.limits.maxSamplerAnisotropy);
	info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable = VK_FALSE;
	info.compareOp = VK_COMPARE_OP_ALWAYS;

	if (vkCreateSampler(device.GetLogicalDevice(), &info, nullptr, &textureSampler) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to create texture sampler.");

	// Publish to cache
	{
		std::lock_guard<std::mutex> lock(g_samplerCacheMutex);
		g_samplerCache[key] = textureSampler;
	}
}

void Material::CreateDescriptorSetLayout()
{
	descriptorSetLayout = Material::GetDescriptorSetLayoutStatic(device);
}

void Material::AllocateAndWriteDescriptorSet()
{
	assert(externalDescriptorPool != VK_NULL_HANDLE && "Material needs a shared descriptor pool");

	// Allocate descriptor set
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = externalDescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	if (vkAllocateDescriptorSets(device.GetLogicalDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to allocate descriptor set.");

	// Bind sampler and image view
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = textureSampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device.GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
}
