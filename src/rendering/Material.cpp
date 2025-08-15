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
		return std::memcmp(this, &other, sizeof(SamplerCacheKey)) == 0;
	}
};

struct SamplerCacheKeyHash 
{
	size_t operator()(const SamplerCacheKey& k) const 
	{
		return std::hash<float>()(k.maxAnisotropy)
			^ (std::hash<int>()(k.minFilter) << 1)
			^ (std::hash<int>()(k.magFilter) << 2)
			^ (std::hash<int>()(k.addressModeU) << 3)
			^ (std::hash<int>()(k.addressModeV) << 4)
			^ (std::hash<int>()(k.addressModeW) << 5);
	}
};

static std::unordered_map<SamplerCacheKey, VkSampler, SamplerCacheKeyHash> g_samplerCache;

struct TextureStagingRing
{
	VulkanDevice* device = nullptr;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	uint8_t* mappedPtr = nullptr;
	VkDeviceSize size = 0;
	VkDeviceSize head = 0;
};

static TextureStagingRing g_texRing;
static std::mutex g_texRingMutex;
static bool g_texRingInited = false;

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

void Material::InitTextureStaging(VulkanDevice& device, VkDeviceSize sizeBytes)
{
	std::lock_guard<std::mutex> lock(g_texRingMutex);

	if (g_texRingInited)
		return;

	g_texRing.device = &device;
	g_texRing.size = sizeBytes;
	
	device.CreateBuffer(sizeBytes,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_texRing.buffer, g_texRing.memory);

	vkMapMemory(device.GetLogicalDevice(), g_texRing.memory, 0, sizeBytes, 0, reinterpret_cast<void**>(&g_texRing.mappedPtr));

	g_texRing.head = 0;
	g_texRingInited = true;
}

void Material::DestroyTextureStaging(VulkanDevice& device)
{
	std::lock_guard<std::mutex> lock(g_texRingMutex);
	if (!g_texRingInited) return;

	if (g_texRing.mappedPtr) {
		vkUnmapMemory(device.GetLogicalDevice(), g_texRing.memory);
		g_texRing.mappedPtr = nullptr;
	}
	if (g_texRing.buffer) {
		vkDestroyBuffer(device.GetLogicalDevice(), g_texRing.buffer, nullptr);
		g_texRing.buffer = VK_NULL_HANDLE;
	}
	if (g_texRing.memory) {
		vkFreeMemory(device.GetLogicalDevice(), g_texRing.memory, nullptr);
		g_texRing.memory = VK_NULL_HANDLE;
	}

	g_texRing.device = nullptr;
	g_texRing.size = 0;
	g_texRing.head = 0;
	g_texRingInited = false;
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
	// ---- Load pixels ----
	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		std::cerr << "[stb_image] Failed to load: " << path
			<< "\nReason: " << stbi_failure_reason() << "\n";
		throw std::runtime_error("[Material] Failed to load texture image.");
	}

	VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) *
		static_cast<VkDeviceSize>(texHeight) * 4ull;

	// Compute mip count and store for view/sampler
	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	textureMipLevels = mipLevels;

	// Ensure staging ring exists (128 MB default)
	{
		std::lock_guard<std::mutex> lock(g_texRingMutex);
		if (!g_texRingInited) {
			Material::InitTextureStaging(device, 128ull * 1024ull * 1024ull);
		}
	}

	// ---- Create optimal-tiled image (with all mips) ----
	VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device.GetLogicalDevice(), &imageInfo, nullptr, &textureImage) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to create texture image.");

	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(device.GetLogicalDevice(), textureImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device.FindMemoryType(
		memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(device.GetLogicalDevice(), &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to allocate image memory.");

	vkBindImageMemory(device.GetLogicalDevice(), textureImage, textureImageMemory, 0);

	// ---- Upload level 0 (ring path or one-off fallback) ----
	if (imageSize > g_texRing.size) {
		// Fallback: one-off staging buffer for very large textures
		VkBuffer stagingBuf = VK_NULL_HANDLE;
		VkDeviceMemory stagingMem = VK_NULL_HANDLE;

		device.CreateBuffer(imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuf, stagingMem);

		void* p = nullptr;
		vkMapMemory(device.GetLogicalDevice(), stagingMem, 0, imageSize, 0, &p);
		std::memcpy(p, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device.GetLogicalDevice(), stagingMem);

		device.CopyBufferToImage(stagingBuf, /*bufferOffset*/ 0, textureImage,
			static_cast<uint32_t>(texWidth),
			static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(device.GetLogicalDevice(), stagingBuf, nullptr);
		vkFreeMemory(device.GetLogicalDevice(), stagingMem, nullptr);
	}
	else {
		// Ring-buffer path
		VkDeviceSize offset = 0;
		{
			std::lock_guard<std::mutex> ringLock(g_texRingMutex);

			auto alignUp = [](VkDeviceSize v, VkDeviceSize a) { return (v + (a - 1)) & ~(a - 1); };
			const VkDeviceSize alignment = 256;
			VkDeviceSize head = alignUp(g_texRing.head, alignment);

			if (head + imageSize > g_texRing.size) {
				// Wrap: wait for in-flight copies to finish, then reuse ring (lock queue to avoid threading warnings)
				{
					std::lock_guard<std::mutex> qlock(device.queueSubmitMutex);
					vkQueueWaitIdle(device.GetGraphicsQueue());
				}
				head = 0;
			}

			offset = head;
			// Safety check (debug)
			assert(offset + imageSize <= g_texRing.size && "staging ring overflow");

			std::memcpy(g_texRing.mappedPtr + offset, pixels, static_cast<size_t>(imageSize));
			g_texRing.head = head + imageSize;
		}

		device.CopyBufferToImage(g_texRing.buffer, offset, textureImage,
			static_cast<uint32_t>(texWidth),
			static_cast<uint32_t>(texHeight));
	}

	// Pixels no longer needed on CPU
	stbi_image_free(pixels);

	// ---- Generate mipmaps & transition all levels to SHADER_READ_ONLY ----
	Material::GenerateMipmapsNow(device, textureImage, VK_FORMAT_R8G8B8A8_SRGB,
		texWidth, texHeight, mipLevels);
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
	viewInfo.subresourceRange.levelCount = textureMipLevels;
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

	// Fast path: reuse
	{
		std::lock_guard<std::mutex> lock(g_samplerCacheMutex);
		auto it = g_samplerCache.find(key);
		if (it != g_samplerCache.end()) { textureSampler = it->second; return; }
	}

	// Create new
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
	info.minLod = 0.0f;
	info.maxLod = static_cast<float>(textureMipLevels);

	if (vkCreateSampler(device.GetLogicalDevice(), &info, nullptr, &textureSampler) != VK_SUCCESS)
		throw std::runtime_error("[Material] Failed to create texture sampler.");

	// Publish
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

void Material::GenerateMipmapsNow(VulkanDevice& device, VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	// Check linear blit support
	VkFormatProperties props{};
	vkGetPhysicalDeviceFormatProperties(device.GetPhysicalDevice(), format, &props);
	if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("GenerateMipmaps: linear blit not supported for this format");
	}

	// Allocate one-time command buffer from the thread-local pool
	VkCommandPool threadPool = device.GetThreadCommandPool()->GetOrCreatePoolForCurrentThread();

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = threadPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(device.GetLogicalDevice(), &allocInfo, &cmd);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipW = texWidth;
	int32_t mipH = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
		// 1) Make previous level (i-1) a SRC for blit
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);

		// 2) Make current level (i) a DST for blit (THIS WAS MISSING)
		barrier.subresourceRange.baseMipLevel = i;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);

		// 3) Blit (i-1) -> (i)
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipW, mipH, 1 };
		blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 };

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { std::max(1, mipW / 2), std::max(1, mipH / 2), 1 };
		blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 };

		vkCmdBlitImage(cmd,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit, VK_FILTER_LINEAR);

		// 4) Transition previous level (i-1) to SHADER_READ_ONLY
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);

		mipW = std::max(1, mipW / 2);
		mipH = std::max(1, mipH / 2);
	}

	// Last mip: DST -> SHADER_READ_ONLY
	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	{
		std::lock_guard<std::mutex> lock(device.queueSubmitMutex);
		vkQueueSubmit(device.GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
		vkQueueWaitIdle(device.GetGraphicsQueue());
	}

	vkFreeCommandBuffers(device.GetLogicalDevice(), threadPool, 1, &cmd);
}
