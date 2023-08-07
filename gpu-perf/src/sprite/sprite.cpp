/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    point_sprite.cpp
*  @brif:    point sprite
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>
#include <glad/glad.h>
#include "vmath.h"
#include "GLSLProgram.h"
#include "Texture.h"
#include "Node.h"
#include "ObjectFactory.h"
#include "Window.h"

#define NUM_STARS 3000
#define ATTRIB_COUNT 2
struct star_t {
    vmath::vec3 position;
    vmath::vec3 color;
};

class sprite: public Node
{
public:
    sprite();
protected:
    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();
protected:

    void setStarData(star_t *star, int count);
    void enableVao();
private:
    GLSLProgram program;
    GLuint starTexture;
    GLuint starBuffer;
    GLuint attriIndex[ATTRIB_COUNT] = {0};
    const char *attribNames[ATTRIB_COUNT] = {"position", "color"};
    std::function<void(star_t *stars, int count)> cb;
};

sprite::sprite()
{
    nodeName = "sprite";
}

bool sprite::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200)
        return false;

    cb = [&](star_t *stars, int count) {
        setStarData(stars, count);
    };
    bool ok = program.build("../media/shaders/points/pointsprite.vs.glsl",
                            "../media/shaders/points/pointsprite.fs.glsl");
    if (!ok)
        return false;
    program.use();
    for (int i = 0; i < ATTRIB_COUNT; i++) {
        attriIndex[i] = static_cast<GLuint>(program.getAttriLocation(attribNames[i]));
    }

    starTexture = Texture::loadKtxImage("../media/textures/sprite1.image");
    glGenBuffers(1, &starBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, starBuffer);
    mapBufferDataCallback<star_t>(NUM_STARS, cb);
    enableVao();

    return true;
}

void sprite::render(double currentTime, double difTime)
{
    const GLfloat color[] = {0.0f, 0.0f, 0.0f, 0.0f};

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(color[0], color[1], color[2], color[3]);

    static float angle = 0.0;
    static float fstep = 0.0;

    vmath::mat4 proj_matrix = vmath::perspective(50.0f, getWindowRatio(), 0.1f, 3000.0f);

    program.use();
    program.setUniform1f("time", static_cast<GLfloat>(currentTime));
    program.setUniformMatrix4fv("proj_matrix", proj_matrix);
    program.setUniform1f("fstep", fstep);
    program.setUniform1f("angle", angle);

    angle += static_cast<GLfloat>(difTime * 1.66);
    if (angle >= 360)
        angle -= 360;
    if (angle < 0)
        angle += 360;

    fstep += static_cast<GLfloat>(difTime * 0.006);
    if (fstep >= 1)
        fstep -= 1;
    if (fstep < 0)
        fstep += 1;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    enableVao();
    glBindTexture(GL_TEXTURE_2D, starTexture);
    glDrawArrays(GL_POINTS, 0, NUM_STARS);
    glDisable(GL_BLEND);
}

void sprite::shutdown()
{
    program.clear();
    glDisable(GL_BLEND);
    glDeleteTextures(1, &starTexture);
    glDeleteBuffers(1, &starBuffer);
}

void sprite::setStarData(star_t *stars, int count)
{
    auto rand = []()->float {
        return (std::rand() % 100) * 0.01f;
    };

    for (int i = 0; i < count; i++) {
        stars[i].position[0] = (rand() * 2.0f - 1.0f) * 50.0f;
        stars[i].position[1] = (rand() * 2.0f - 1.0f) * 50.0f;
        stars[i].position[2] = rand();

        stars[i].color[0] = 0.5f + rand() * 0.5f;
        stars[i].color[1] = 0.3f + rand() * 0.5f;
        stars[i].color[2] = 0.5f + rand() * 0.5f;
    }
}

void sprite::enableVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, starBuffer);
    glVertexAttribPointer(attriIndex[0], 3, GL_FLOAT, GL_FALSE, sizeof(star_t), nullptr);
    glVertexAttribPointer(attriIndex[1], 3, GL_FLOAT, GL_FALSE, sizeof(star_t), reinterpret_cast<void *>(sizeof(vmath::vec3)));
    glEnableVertexAttribArray(attriIndex[0]);
    glEnableVertexAttribArray(attriIndex[1]);
}

REGISTER_OBJECT(Node, sprite)
