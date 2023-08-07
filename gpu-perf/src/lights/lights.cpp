/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    lights.cpp
*  @brif:    lights cpp
*
*  @date:    25/10/2021
*  @Author:  bird.du
*
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <math.h>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Node.h"
#include "vmath.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GLSLProgram.h"
#include "ObjectFactory.h"
#include "Model.h"
#include "Texture.h"
#define ATTRIB_COUNT 2

enum Light_Type{
    Light_Cartoon = 0,
    Light_Diffuse,
    Light_Gouraud,
    Light_Phong,
    Light_Blinn_Phong,
};

static const GLubyte toon_tex_data[] =
{
    100, 100, 100, 0,
    200, 200, 200, 0,
    255, 255, 255, 0,
};

inline GLfloat truncateDeg(GLfloat deg)
{
    if (deg >= 360.f)
        return deg - 360.f;
    else
        return deg;
}

class lights : public Node
{
public:
    lights()
    {
        nodeName = "lights";
    }
protected:
    bool startup() override;
    void render(double currentTime, double difTime) override;
    void shutdown() override;
    void onResized(int width, int height)override;

protected:
    void makeModel();
    void makeToonTex();
    void setUniform();

private:
    const glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    const glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 1000.0f);
private:
    GLSLProgram program;
    std::unique_ptr<Model> model;
    GLuint textureId = 0;
    GLint lightOperation = Light_Cartoon;
    double lightTimer = 0;
    float angle = 0.0f;
    glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraZAxis = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraXAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 cameraYAxis = glm::vec3(0.0f, 1.0f, 0.0f);
};

bool lights::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200) {
        return false;
    }

    bool ok = program.build("../media/shaders/lights/chicken_vs.glsl", \
                            "../media/shaders/lights/chicken_fs.glsl");
    if (!ok) {
        return false;
    }
    makeModel();
    makeToonTex();
    setUniform();
    lightTimer = PerfWindow::perfGetTime();
    angle = 0.0f;

    return true;
}

void lights::render(double currentTime, double difTime)
{
    (void)currentTime;
    (void)difTime;
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.6f, 0.6f, 0.6f, 0.6f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program.use();
    if ((PerfWindow::perfGetTime() - lightTimer) >= 4) {
        lightOperation++;
        if (lightOperation > Light_Blinn_Phong) {
            lightOperation = Light_Cartoon;
        }
        program.setUniform1i("useCartoon", lightOperation);
        lightTimer = PerfWindow::perfGetTime();
    }
    glm::mat4 trans = glm::mat4(1.0);
    angle = angle + static_cast<float>(30 * difTime);
    if (angle >= 360.0f) {
        angle = 0;
    }
    trans = glm::rotate(trans, glm::radians(angle),glm::vec3(0.0, 1.0, 0.0));
    program.setUniformMatrix4fv("trans", glm::value_ptr(trans));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureId);
    model->renderModel();
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void lights::shutdown()
{
    program.clear();
    model->clearModel();
    glDeleteTextures(1, &textureId);
}

void lights::onResized(int width, int height)
{
    glViewport(0, 0, width, height);
    Node::onResized(width, height);
}

void lights::makeModel()
{
    const char *attribNames[ATTRIB_COUNT] = {"inPos", "inNormal"};
    GLint componentNums[ATTRIB_COUNT] = {3, 3};
    GLuint dataTypes[ATTRIB_COUNT] = {GL_FLOAT, GL_FLOAT};
    GLubyte isNormalizeds[ATTRIB_COUNT] = {GL_FALSE, GL_FALSE};
    GLint strides[ATTRIB_COUNT] = {8 * sizeof(GLfloat), 8 * sizeof(GLfloat)};
    GLint bitOffsets[ATTRIB_COUNT] = {0, 5 * sizeof(GLfloat)};

    std::vector<VertexFormat> attribs(ATTRIB_COUNT);
    program.use();
    for (uint i = 0; i < ATTRIB_COUNT; i++) {
        attribs[i].location = static_cast<uint>(program.getAttriLocation(attribNames[i]));
        attribs[i].componentNum = componentNums[i];
        attribs[i].dataType = dataTypes[i];
        attribs[i].isNormalized = isNormalizeds[i];
        attribs[i].stride = strides[i];
        attribs[i].bitOffset = bitOffsets[i];
    }
    model.reset(new Model(attribs));
    model->loadModel("../media/objects/cyborg.obj");
}

void lights::makeToonTex()
{
    if (textureId == 0) {
        glGenTextures(1, &textureId);
    }
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sizeof(toon_tex_data) / 4, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, toon_tex_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void lights::setUniform()
{
    glm::mat4 trans = glm::mat4(1.0);
    glm::mat4 view = glm::lookAt(viewPos, viewPos + glm::normalize(cameraZAxis), cameraYAxis);
    glm::mat4 projection = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(45.0f),\
                                  static_cast<float>(getWidth())/static_cast<float>(getHeight()),\
                                  0.1f, 10000.0f);
    program.use();
    program.setUniformMatrix4fv("trans", glm::value_ptr(trans));
    program.setUniformMatrix4fv("view", glm::value_ptr(view));
    program.setUniformMatrix4fv("projection", glm::value_ptr(projection));
    program.setUniform3f("lightColor", lightColor.x, lightColor.y, lightColor.z);
    program.setUniform3f("lightPos", lightPos.x, lightPos.y, lightPos.z);
    program.setUniform3f("viewPos", viewPos.x, viewPos.y, viewPos.z);
    program.setUniform1i("texture02", 1);
    program.setUniform1i("useCartoon", static_cast<int>(lightOperation));
}

REGISTER_OBJECT(Node, lights)

