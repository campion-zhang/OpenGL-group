/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    material.cpp
*  @brif:    diff material
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <memory>
#include "vmath.h"
#include <glad/glad.h>
#include "GLSLProgram.h"
#include "Texture.h"
#include "Node.h"
#include "Model.h"
#include "ObjectFactory.h"

#define ATTRIB_COUNT 2

class material: public Node
{
public:
    material();
protected:
    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();
protected:
    GLSLProgram program;
    GLuint tex_envmap;
    GLuint envmaps[4];
    int envmap_index;
    std::unique_ptr<Model> model;
};

material::material()
{
    nodeName = "material";
}

bool material::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200)
        return false;

    const char *attribNames[ATTRIB_COUNT] = {"position", "inNormal"};
    GLint componentNums[ATTRIB_COUNT] = {3, 3};
    GLuint dataTypes[ATTRIB_COUNT] = {GL_FLOAT, GL_FLOAT};
    GLubyte isNormalizeds[ATTRIB_COUNT] = {GL_FALSE, GL_FALSE};
    GLint strides[ATTRIB_COUNT] = {8 * sizeof(GLfloat), 8 * sizeof(GLfloat)};
    GLint bitOffsets[ATTRIB_COUNT] = {0, 5 * sizeof(GLfloat)};

    bool ok = program.build("../media/shaders/material/render.vs.glsl",
                            "../media/shaders/material/render.fs.glsl");
    if (!ok)
        return false;

    envmap_index = 0;
    envmaps[0] = Texture::loadKtxImage("../media/textures/envmaps/1.image");
    envmaps[1] = Texture::loadKtxImage("../media/textures/envmaps/2.image");
    envmaps[2] = Texture::loadKtxImage("../media/textures/envmaps/3.image");
    tex_envmap = envmaps[envmap_index];

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

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

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    return true;
}

void material::render(double currentTime, double difTime)
{
    (void)difTime;
    static const GLfloat gray[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    static const GLfloat ones[] = { 1.0f };

    double temp = (totalTime + 2) / 3;
    envmap_index = static_cast<GLint>(lastTime / temp);
    tex_envmap = envmaps[envmap_index];

    glClearColor(gray[0], gray[1], gray[2], gray[3]);
    glClearDepthf(ones[0]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (envmap_index > 2 )
        tex_envmap = 0;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_envmap);

    program.use();

    float f = static_cast<GLfloat>(currentTime * 1.5);

    auto model_matrix = vmath::scale(42.0f);
    auto view_position = vmath::vec3(cosf(f * 0.35f) * 120.0f, cosf(f * 0.4f) * 1.0f,
                                     sinf(f * 0.35f) * 120.0f);
    auto view_matrix = vmath::lookat(view_position, vmath::vec3(0.0f, .0f, 0.0f), vmath::vec3(0.0f,
                                                                                              1.0f, 0.0f));
    auto mv_matrix = view_matrix * model_matrix;
    auto proj_matrix = vmath::perspective(45.0f,  static_cast<GLfloat>(getWidth()) / static_cast<GLfloat>(getHeight()),
                                          0.1f,
                                          1000.0f);

    program.setUniformMatrix4fv("mv_matrix", mv_matrix);
    program.setUniformMatrix4fv("proj_matrix", proj_matrix);

    model->renderModel();
}

void material::shutdown()
{
    program.clear();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0);
    glDeleteTextures(4, envmaps);
}

REGISTER_OBJECT(Node, material)
