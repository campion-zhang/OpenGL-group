#pragma once

#include <vector>

#include <glad/glad.h>
#include <vmath.h>
#include <globaldefine.h>

class Mesh
{
public:
    Mesh();
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    ~Mesh();
public:
    void createMesh(GLfloat *vertices, unsigned short *indices, unsigned int numOfVertices,
                    unsigned int numOfIndices);
    void renderMesh();
    void clearMesh();

    vmath::vec3 getMinPosition()const;
    vmath::vec3 getMaxPosition()const;
    void setAttribLocation(const std::vector<VertexFormat> &tempAttrib);
private:
    void enableVao();
private:
    GLuint vbo, ibo;
    GLuint tfbo;
    GLsizei count;
    vmath::vec3 minPos, maxPos;

    std::vector<GLfloat> meshVertices;
    std::vector<GLushort> meshIndices;
    std::vector<VertexFormat> attribLocation;
};
