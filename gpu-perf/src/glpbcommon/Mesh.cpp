#include <stdlib.h>
#include <string.h>
#include <memory>
#include <limits>
#include <algorithm>
#include "Mesh.h"

Mesh::Mesh()
{
    vbo = 0;
    ibo = 0;
    count = 0;
    tfbo = 0;
}

vmath::vec3 Mesh::getMinPosition()const
{
    return minPos;
}

vmath::vec3 Mesh::getMaxPosition()const
{
    return maxPos;
}

void Mesh::createMesh(GLfloat *vertices, unsigned short *indices, unsigned int numOfVertices,
                      unsigned int numOfIndices)
{
    count = static_cast<int>(numOfIndices);

    meshVertices.clear();
    meshVertices.resize(numOfVertices);
    memcpy(&meshVertices[0], vertices, static_cast<int>(numOfVertices * sizeof(GLfloat)));

    meshIndices.clear();
    meshIndices.resize(numOfIndices);
    memcpy(&meshIndices[0], indices, static_cast<int>(numOfIndices * sizeof(unsigned short)));

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * numOfIndices, indices, GL_STATIC_DRAW);

    glGenBuffers(1, &vbo);
    enableVao();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    minPos = vmath::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max());
    maxPos = vmath::vec3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(),
                         std::numeric_limits<float>::min());

    for (unsigned int i = 0; i < numOfIndices / 8; i++) {
        float x = vertices[8 * i + 0];
        float y = vertices[8 * i + 1];
        float z = vertices[8 * i + 2];
        minPos[0] = std::min<float>(x, minPos[0]);
        minPos[1] = std::min<float>(y, minPos[1]);
        minPos[2] = std::min<float>(z, minPos[2]);
        maxPos[0] = std::max<float>(x, maxPos[0]);
        maxPos[1] = std::max<float>(y, maxPos[1]);
        maxPos[2] = std::max<float>(z, maxPos[2]);
    }
}

void Mesh::renderMesh()
{
    enableVao();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::clearMesh()
{
    if (ibo != 0) {
        glDeleteBuffers(1, &ibo);
        ibo = 0;
    }

    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (tfbo != 0) {
        glDeleteBuffers(1, &tfbo);
        tfbo = 0;
    }
    count = 0;
}

Mesh::~Mesh()
{
    clearMesh();
}

void Mesh::enableVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLuint>(sizeof(meshVertices[0]) * meshVertices.size()), meshVertices.data(), GL_STATIC_DRAW);
    for (uint i = 0; i < attribLocation.size(); i++) {
        glVertexAttribPointer(attribLocation[i].location, attribLocation[i].componentNum, \
                              attribLocation[i].dataType, attribLocation[i].isNormalized, \
                              attribLocation[i].stride, reinterpret_cast<void *>(attribLocation[i].bitOffset));
        glEnableVertexAttribArray(attribLocation[i].location);
    }
}

void Mesh::setAttribLocation(const std::vector<VertexFormat> &tempAttrib)
{
    attribLocation = tempAttrib;
}
