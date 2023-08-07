/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    skybox.cpp
*  @brif:    skybox
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <vector>
#include <array>
#include <memory>
#include <glad/glad.h>
#include "vmath.h"
#include "GLSLProgram.h"
#include "Texture.h"
#include "ObjectFactory.h"
#include "Node.h"
#include "vertex_roll.h"

#define BALL_ATTRIB_COUNT 2
#define SKYBOX_ATTRIB_COUNT 2

static GLfloat vertices[] = {
    -1.0, -1.0, 1.0,  1.0, -1.0, 1.0,
     1.0, -1.0, 1.0, -1.0, -1.0, 1.0,
    -1.0,  1.0, 1.0,  1.0,  1.0, 1.0,
     1.0,  1.0, 1.0, -1.0,  1.0, 1.0
};

class skybox: public Node
{
public:
    skybox();
protected:
    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();
protected:
    void enableBallVao();
    void enableSkyboxVao();
protected:
    GLSLProgram programRender;
    GLSLProgram programSkyBox;

    GLuint  tex_envmap;
    GLuint  envmaps[3];
    int envmap_index;

    GLuint vbo_ball = 0;
    GLuint vbo_SkyBox = 0;

    GLuint ballAttriIndex[BALL_ATTRIB_COUNT] = {0};
    const char *ballAttribNames[BALL_ATTRIB_COUNT] = {"position", "normal"};
    GLuint skyboxAttriIndex[SKYBOX_ATTRIB_COUNT] = {0};
    const char *skyboxAttribNames[SKYBOX_ATTRIB_COUNT] = {"inPos", "inTexcoord"};
};

skybox::skybox()
{
    nodeName = "skybox";
}

bool skybox::startup()
{
    if(PerfWindow::get()->getGLESVersion() < 200) {
        return false;
    }

    if(!programRender.build("../media/shaders/skybox/render.vs.glsl", "../media/shaders/skybox/render.fs.glsl")) {
        return false;
    }

    setAttribLocation(ballAttriIndex, ballAttribNames, programRender, BALL_ATTRIB_COUNT);

    if(!programSkyBox.build("../media/shaders/skybox/skybox.vs.glsl", "../media/shaders/skybox/skybox.fs.glsl")) {
        return false;
    }

    setAttribLocation(skyboxAttriIndex, skyboxAttribNames, programSkyBox, SKYBOX_ATTRIB_COUNT);

    std::array<std::string, 6> pictures =
    {
        "../media/textures/SanFrancisco3/posx.jpg",
        "../media/textures/SanFrancisco3/negx.jpg",
        "../media/textures/SanFrancisco3/posy.jpg",
        "../media/textures/SanFrancisco3/negy.jpg",
        "../media/textures/SanFrancisco3/posz.jpg",
        "../media/textures/SanFrancisco3/negz.jpg"
    };

    if(!Texture::loadCubeMap(pictures, &tex_envmap)) {
        return false;
    }
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glGenBuffers(1, &vbo_SkyBox);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_SkyBox);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    enableSkyboxVao();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vbo_ball);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ball);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_roll) + sizeof(normals_roll), nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_roll), vertex_roll);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertex_roll), sizeof(normals_roll), normals_roll);
    enableBallVao();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void skybox::render(double currentTime, double difTime)
{
    (void)difTime;
    static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    static const GLfloat ones[] = { 1.0f };
    const float tf = static_cast<GLfloat>(currentTime * 0.3);

    vmath::mat4 proj_matrix = vmath::perspective(60.0f, getWindowRatio(), 0.1f, 1000.0f);
    vmath::mat4 view_matrix = vmath::lookat(vmath::vec3(15.0f * sinf(tf), 0.0f, 15.0f * cosf(tf)),
                                            vmath::vec3(0.0f, 0.0f, 0.0f),
                                            vmath::vec3(0.0f, 1.0f, 0.0f));
    vmath::mat4 zoom_matrix = vmath::scale(0.10f, 0.10f, 0.10f);
    vmath::mat4 mv_matrix = view_matrix * zoom_matrix *
                            //vmath::rotate(tf, 1.0f, 0.0f, 0.0f) *
                            //vmath::rotate((float)difTime*0.1f, 0.0f, 1.0f, 0.0f) *
                            vmath::translate(0.0f, 0.0f, 0.0f);

    glClearColor(gray[0], gray[1], gray[2], gray[3]);
    glClearDepthf(ones[0]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_envmap);

    glViewport(0, 0, getWidth(), getHeight());

    programSkyBox.use();
    programSkyBox.setUniformMatrix4fv("view_matrix", view_matrix.operator float * ());

    enableSkyboxVao();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    programRender.use();
    programRender.setUniformMatrix4fv("mv_matrix", mv_matrix);
    programRender.setUniformMatrix4fv("proj_matrix", proj_matrix);

    enableBallVao();
    glDrawArrays(GL_TRIANGLES, 0, numofroll);
}

void skybox::shutdown()
{
    programRender.clear();
    programSkyBox.clear();

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glDeleteBuffers(1, &vbo_ball);
    glDeleteBuffers(1, &vbo_SkyBox);
    glDeleteTextures(3, envmaps);
    glDeleteTextures(1, &tex_envmap);
}

void skybox::enableBallVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ball);
    glEnableVertexAttribArray(ballAttriIndex[0]);
    glVertexAttribPointer(ballAttriIndex[0], 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(ballAttriIndex[1]);
    glVertexAttribPointer(ballAttriIndex[1], 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(vertex_roll)));
}

void skybox::enableSkyboxVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo_SkyBox);
    glEnableVertexAttribArray(skyboxAttriIndex[0]);
    glVertexAttribPointer(skyboxAttriIndex[0], 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(skyboxAttriIndex[1]);
    glVertexAttribPointer(skyboxAttriIndex[1], 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), BUFFER_OFFSET(3 * sizeof(GLfloat)));
}

REGISTER_OBJECT(Node, skybox)


