#include "AsyncModelLoader.h"

void AsyncModelLoader::RequestLoad(const std::string& path, VulkanDevice& device, VkDescriptorPool materialPool)
{
	materialPoolCaptured = materialPool;
	if (loading) return;

	loading = true;
	result.reset();

	worker = std::thread(&AsyncModelLoader::LoadTask, this, path, &device);
	worker.detach();
}

bool AsyncModelLoader::isLoading() const
{
	return loading;
}

std::optional<AsyncModelLoader::Result> AsyncModelLoader::GetResult()
{
	std::lock_guard<std::mutex> lock(resultMutex);
	if (result)
	{
		auto res = std::move(result);
		result.reset();
		loading = false;
		return res;
	}
	return std::nullopt;
}

void AsyncModelLoader::LoadTask(std::string path, VulkanDevice* device)
{
	MeshBatch tempBatch;
	Scene tempScene;

	// Create a command pool for the thread
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = device->FindQueueFamilies(device->GetPhysicalDevice()).graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VkCommandPool threadCommandPool;
	if (vkCreateCommandPool(device->GetLogicalDevice(), &poolInfo, nullptr, &threadCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool for async loading");
	}

	tempBatch.SetCustomCommandPool(threadCommandPool); // You’ll need to support this in MeshBatch

	bool success = ModelLoader::LoadModel(path, *device, tempBatch, tempScene, materialPoolCaptured);

	vkDestroyCommandPool(device->GetLogicalDevice(), threadCommandPool, nullptr);

	{
		std::lock_guard<std::mutex> lock(resultMutex);
		result = Result{ std::move(tempBatch), std::make_shared<Scene>(std::move(tempScene)), success };
	}
}