#ifndef MODEL_INSTANCE_H
#define MODEL_INSTANCE_H

#include <memory>
#include <glm/glm.hpp>
#include "MeshBatch.h"
#include "Mesh.h"
#include "Material.h"

struct ModelInstance
{
	glm::mat4 transform{};
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	uint32_t meshIndex = 0;

	ModelInstance(const glm::mat4& transform, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, uint32_t meshIndex)
		: transform(transform), mesh(mesh), material(material), meshIndex(meshIndex) {}
};

#endif // !MODEL_INSTANCE_H
