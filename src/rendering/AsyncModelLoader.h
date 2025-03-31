#ifndef ASYNC_MODEL_LOADER_H
#define ASYNC_MODEL_LOADER_H

#include <memory>
#include <string>
#include <mutex>
#include <optional>
#include <thread>
#include <atomic>

#include "Scene.h"
#include "ModelLoader.h"
#include "MeshBatch.h"

class AsyncModelLoader
{
public:
	struct Result
	{
		MeshBatch meshBatch;
		std::shared_ptr<Scene> scene;
		bool success;
	};

	AsyncModelLoader() = default;

	void RequestLoad(const std::string& path, VulkanDevice& device);
	bool isLoading() const;
	std::optional<Result> GetResult();

private:
	void LoadTask(std::string path, VulkanDevice* device);

	std::atomic<bool> loading = false;
	std::mutex resultMutex;
	std::optional<Result> result;
	std::thread worker;
};

#endif // !ASYNC_MODEL_LOADER_H