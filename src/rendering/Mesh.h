#ifndef MESH_H
#define MESH_H

#include <memory>
#include <glm/glm.hpp>
#include "Vertex.h"
#include "MeshBatch.h"

class Mesh
{
public:
    Mesh(VkBuffer vertexBuffer, VkDeviceMemory vertexMemory,
        VkBuffer indexBuffer, VkDeviceMemory indexMemory,
        const MeshBatch::MeshRange& range);

    ~Mesh();

    void Bind(VkCommandBuffer commandBuffer) const;
    void Draw(VkCommandBuffer commandBuffer) const;

private:
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    MeshBatch::MeshRange range;
};


#endif // !MESH_H