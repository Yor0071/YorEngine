#ifndef MODEL_INSTANCE_H
#define MODEL_INSTANCE_H

#include <memory>

#include "Mesh.h"

struct ModelInstance
{
	std::shared_ptr<Mesh> mesh;
	glm::mat4 transform = glm::mat4(1.0f);
};

#endif // !MODEL_INSTANCE_H
