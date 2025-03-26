#include "ModelLoader.h"
#include <iostream>

bool ModelLoader::LoadModel(const std::string& filepath, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filepath,
		aiProcess_Triangulate |
		aiProcess_GenNormals |
		aiProcess_ImproveCacheLocality |
		aiProcess_JoinIdenticalVertices |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_SortByPType |
		aiProcess_FlipWindingOrder |
		aiProcess_FlipUVs
	);

	if (!scene || !scene->HasMeshes()) {
		std::cerr << "[ModelLoader] Failed to load: " << filepath << std::endl;
		return false;
	}

	outVertices.clear();
	outIndices.clear();

	uint32_t vertexOffset = 0;

	for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
		const aiMesh* mesh = scene->mMeshes[m];

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			Vertex vertex{};
			vertex.pos = {
				mesh->mVertices[i].x,
				mesh->mVertices[i].y,
				mesh->mVertices[i].z
			};

			if (mesh->HasNormals()) {
				vertex.color = {
					mesh->mNormals[i].x,
					mesh->mNormals[i].y,
					mesh->mNormals[i].z
				};
			}
			else {
				vertex.color = { 1.0f, 1.0f, 1.0f };
			}

			outVertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; ++j) {
				outIndices.push_back(face.mIndices[j] + vertexOffset);
			}
		}

		vertexOffset += mesh->mNumVertices;
	}

	std::cout << "[ModelLoader] Loaded '" << filepath << "' with "
		<< outVertices.size() << " vertices, "
		<< outIndices.size() << " indices." << std::endl;

	return true;
}
