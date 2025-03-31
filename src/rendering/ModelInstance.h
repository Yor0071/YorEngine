#ifndef MODEL_INSTANCE_H
#define MODEL_INSTANCE_H

#include <memory>
#include <glm/glm.hpp>
#include "MeshBatch.h"
#include "Mesh.h"

struct ModelInstance
{
	glm::mat4 transform{};
	std::shared_ptr<Mesh> mesh;
	uint32_t meshIndex = 0;
};

#endif // !MODEL_INSTANCE_H
