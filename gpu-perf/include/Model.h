#pragma once
#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/config.h>
#include "vmath.h"
#include <globaldefine.h>

class Mesh;
class Texture;

class Model
{
public:
    Model() = delete;
    Model(const std::vector<VertexFormat> &formats);
    Model(const Model &) = delete;
    Model &operator = (const Model &) = delete;
    ~Model() = default;
public:
    void loadModel(const std::string &fileName);
    void renderModel();
    void clearModel();

    vmath::vec3 getMinPosition()const;
    vmath::vec3 getMaxPosition()const;

    void setAttribLocation(const std::vector<VertexFormat> &tempAttrib);
private:
    void loadNode(aiNode *node, const aiScene *scene);
    void loadMesh(aiMesh *mesh, const aiScene *scene);
    void loadMaterials(const aiScene *scene);
private:
    std::vector<Mesh *> meshList;
    std::vector<Texture *> textureList;
    std::vector<unsigned int> meshToTex;
    vmath::vec3 minPosition, maxPosition;
    std::vector<VertexFormat> attribLocation;
};

