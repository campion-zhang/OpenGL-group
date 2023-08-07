/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    cineshader.cpp
*  @brif:    cineshader cpp
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <vmath.h>
#include <glad/glad.h>
#include <Window.h>
#include <Node.h>
#include <GLSLProgram.h>
#include <ObjectFactory.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define ATTRIB_COUNT 1
static float vertices[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f,
};

class shader: public Node
{
public:
    shader();
protected:
    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();
private:
    void enableVertex();
protected:
    GLuint vbo;
    GLSLProgram program;
private:
    GLuint attriIndex[ATTRIB_COUNT] = {0};
    const char *attribNames[ATTRIB_COUNT] = {"inPos"};
};

shader::shader()
{
    nodeName = "shader";
}

bool shader::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200)
        return false;

    bool ret = program.build("../media/shaders/cineshader/render.vs.glsl",
                         "../media/shaders/cineshader/render.fs.glsl");
    if (!ret) {
        return false;
    }
    for (int i = 0; i < ATTRIB_COUNT; i++) {
        attriIndex[i] = static_cast<GLuint>(program.getAttriLocation(attribNames[i]));
    }

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    enableVertex();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void shader::enableVertex()
{
    glVertexAttribPointer(attriIndex[0], 2, GL_FLOAT, GL_FALSE, \
                          2 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(attriIndex[0]);
}

void shader::render(double currentTime, double difTime)
{
    (void)difTime;

    glDisable(GL_BLEND);
    static const GLfloat gray[] = {0.0f, 0.8f, 0.2f, 0.0f};
    glClearColor(gray[0], gray[1], gray[2], gray[3]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    enableVertex();
    program.use();
    program.setUniform1f("uTime", static_cast<float>(currentTime));
    vmath::vec2 size = vmath::vec2(getWidth(), getHeight());
    program.setUniform2fv("texSize", 1, size);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void shader::shutdown()
{
    glDeleteBuffers(1, &vbo);
    program.clear();
    glUseProgram(0);
}

REGISTER_OBJECT(Node, shader)
