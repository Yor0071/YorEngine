#ifndef PENDING_MODEL_H
#define PENDING_MODEL_H

#include <vector>
#include <memory>
#include <mutex>

#include "Mesh.h"
#include "Scene.h"

struct PendingModel
{
	std::vector<std::shared_ptr<Mesh>> meshes;
	Scene tempScene;
	std::string modelPath;
	bool ready = false;
	std::mutex mutex;
	bool failed = false;
};

#endif // !PENDING_MODEL_H
