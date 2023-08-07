/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    fire.cpp
*  @brif:    fire cpp
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <Model.h>
#include <vector>
#include <array>
#include <memory>
#include <glad/glad.h>
#include "vmath.h"
#include "GLSLProgram.h"
#include "Texture.h"
#include "Node.h"
#include "ObjectFactory.h"

#define ATTRIB_COUNT 1
class fire: public Node
{
public:
    fire();
protected:
    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();
protected:
    void enableVao();
protected:
    GLSLProgram program;
    GLuint vbo;
    const GLfloat pos[8] = {
        -0.25, -0.25,
         0.25, -0.25,
        -0.25,  0.25,
         0.25,  0.25,
    };
    GLuint attriIndex[ATTRIB_COUNT] = {0};
    const char *attribNames[ATTRIB_COUNT] = {"inPos"};
};

fire::fire():
    vbo(0)
{
    nodeName = "fire";
}

bool fire::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200)
        return false;

    bool ok = program.build("../media/shaders/fire/fire.vs.glsl", "../media/shaders/fire/fire.fs.glsl");
    if (!ok)
        return false;

    setAttribLocation(attriIndex, attribNames, program, ATTRIB_COUNT);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);
    enableVao();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void fire::render(double currentTime, double difTime)
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDisable(GL_DEPTH_TEST);
    (void)difTime;
    static const GLfloat gray[] = { 0.0f, 0.8f, 0.2f, 0.0f };

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(gray[0], gray[1], gray[2], gray[3]);

    program.use();

    float tf = static_cast<GLfloat>(currentTime);
    program.setUniform1f("iTime", tf);
    program.setUniform2f("iResolution", getWidth() / 2, getHeight() / 2);

    enableVao();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void fire::shutdown()
{
    glDeleteBuffers(1, &vbo);
    program.clear();
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void fire::enableVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(attriIndex[0]);
    glVertexAttribPointer(attriIndex[0], 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
}

REGISTER_OBJECT(Node, fire)

