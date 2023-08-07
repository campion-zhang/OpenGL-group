/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    fill.cpp
*  @brif:    fill cpp
*
*  @date:    09/01/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <array>
#include <memory>
#include "vmath.h"
#include <glad/glad.h>
#include "GLSLProgram.h"
#include "Node.h"
#include "Texture.h"
#include "ObjectFactory.h"

#define ATTRIB_COUNT 3

class fill: public Node
{
    static constexpr int COUNT = 1;
    static constexpr int MAX_SIZE = 6;
public:
    fill();
protected:
    NodeType getNodeType();
    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();
private:
    void perfCheckerTexture(GLsizei width, GLsizei height, int index);
    void initializeVertexData();
    void drawArrays(int count);
    void enableVertex();
protected:
    GLSLProgram program;
    GLuint vbo = 0;
    int currentTexture = 0;
    bool currentDirectionUp = true;
    GLuint textures[MAX_SIZE];

    float vertices[32] = {
        /*x     y     s    t     r    g    b    a  */
        -1.0, -1.0,  0.0, 0.0,  1.0, 0.0, 0.0, 1,
            1.0, -1.0,  1.0, 0.0,  0.0, 1.0, 0.0, 1,
            1.0,  1.0,  1.0, 1.0,  1.0, 0.0, 0.0, 1,
            -1.0,  1.0,  0.0, 1.0,  0.0, 0.0, 1.0, 1,
        };
    GLuint attriIndex[ATTRIB_COUNT] = {0};
    const char *attribNames[ATTRIB_COUNT] = {"position", "coord", "color"};
};

fill::fill()
{
    nodeName = "fill";
}

Node::NodeType fill::getNodeType()
{
    return NodeType_FillRate;
}

void fill::perfCheckerTexture(GLsizei width, GLsizei height, int index)
{
    auto image = new unsigned char[width * height * 4];
    GLint i, j, k;

    k = 0;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            GLubyte color;
            if (((i / 32) ^ (j / 32)) & 1)
                color = 0xff;
            else
                color = 0x0;

            image[k++] = color;
            image[k++] = color;
            image[k++] = color;
            image[k++] = 0xff;
        }
    }

    glBindTexture(GL_TEXTURE_2D, textures[index]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    delete []image;
}

bool fill::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200)
        return false;

    bool ok = program.build("../media/shaders/fill/fill.vs.glsl", "../media/shaders/fill/fill.fs.glsl");
    if (!ok)
        return false;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    for (int i = 0; i < ATTRIB_COUNT; i++) {
        attriIndex[i] = static_cast<GLuint>(program.getAttriLocation(attribNames[i]));
    }
    enableVertex();

    glGenTextures(MAX_SIZE, textures);
    for (int i = 0; i < MAX_SIZE; i++) {
        perfCheckerTexture(static_cast<int>(64 * std::pow(2, i + 1)), \
                           static_cast<int>(64 * std::pow(2, i + 1)), i);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    return true;
}

void fill::enableVertex()
{
    glVertexAttribPointer(attriIndex[0], 2, GL_FLOAT, GL_FALSE, \
                          8 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(attriIndex[0]);
    glVertexAttribPointer(attriIndex[1], 2, GL_FLOAT, GL_FALSE, \
                          8 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(attriIndex[1]);
    glVertexAttribPointer(attriIndex[2], 4, GL_FLOAT, GL_FALSE, \
                          8 * sizeof(float), reinterpret_cast<void *>(4 * sizeof(float)));
    glEnableVertexAttribArray(attriIndex[2]);
}

void fill::drawArrays(int count)
{
    for (int i = 0; i < count; i++)
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void fill::render(double currentTime, double difTime)
{
    (void)difTime;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    enableVertex();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[currentTexture]);
    program.use();
    program.setUniform1f("time", static_cast<float>(currentTime));
    program.setUniform1f("size", currentTexture);
    drawArrays(COUNT);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (currentDirectionUp)
        currentTexture ++;
    else
        currentTexture --;

    if (currentTexture >= MAX_SIZE) {
        currentTexture -= 2;
        currentDirectionUp = false;
    } else if (currentTexture < 0) {
        currentTexture += 2;
        currentDirectionUp = true;
    }
}

void fill::shutdown()
{
    glDeleteTextures(MAX_SIZE, textures);
    glDeleteBuffers(1, &vbo);
    program.clear();

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

REGISTER_OBJECT(Node, fill)

