#include "ModelLoader.h"
#include <iostream>

bool ModelLoader::LoadModel(
    const std::string& path,
    VulkanDevice& device,
    std::vector<ModelInstance>& outInstances
) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_PreTransformVertices |
        aiProcess_FlipUVs |
        aiProcess_ConvertToLeftHanded
    );

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "[ModelLoader] Failed to load: " << path << std::endl;
        return false;
    }

    std::cout << "[ModelLoader] Scene contains " << scene->mNumMeshes << " meshes." << std::endl;

    outInstances.clear();
    ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f), device, outInstances);

    std::cout << "[ModelLoader] Loaded " << outInstances.size() << " mesh instances." << std::endl;

    return true;
}

void ModelLoader::ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform, VulkanDevice& device, std::vector<ModelInstance>& outInstances)
{
    glm::mat4 transform = parentTransform * ConvertMatrix(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        const aiMesh* mesh = scene->mMeshes[meshIndex];

        if (!mesh->HasPositions()) continue;

        auto gpuMesh = ProcessMesh(mesh, device);
        if (!gpuMesh) continue;

        ModelInstance instance;
        instance.mesh = gpuMesh;
        instance.transform = transform;
        outInstances.push_back(instance);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, transform, device, outInstances);
    }
}

std::shared_ptr<Mesh> ModelLoader::ProcessMesh(const aiMesh* mesh, VulkanDevice& device) {
    if (mesh->mNumVertices == 0) return nullptr;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex v{};
        v.pos = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        if (mesh->HasNormals()) {
            v.color = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }
        else {
            v.color = { 1.0f, 1.0f, 1.0f };
        }

        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return std::make_shared<Mesh>(device, vertices, indices);
}

glm::mat4 ModelLoader::ConvertMatrix(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}
