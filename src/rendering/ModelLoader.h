#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <vector>
#include <string>

#include <glm/glm.hpp>

#include "../third_party/assimp/include/assimp/Importer.hpp"
#include "../third_party/assimp/include/assimp/scene.h"
#include "../third_party/assimp/include/assimp/postprocess.h"

#include "Vertex.h"

class ModelLoader
{
public:
	static bool LoadModel(const std::string& filepath, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);

private:

};

#endif // !MODEL_LOADER_H