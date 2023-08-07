/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    wave.cpp
*  @brif:    water wave
*
*  @date:    05/10/2021
*  @Author:  bird.du
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <vector>
#include <array>
#include <memory>
#include <glad/glad.h>
#include "stb_image.h"
#include "vmath.h"
#include "GLSLProgram.h"
#include "Texture.h"
#include "ObjectFactory.h"
#include "vmath.h"
#include "Node.h"

#define ATTRIB_COUNT 2
class TextureWrapper
{
public:
    bool init(const char *filename, const GLfloat *data, int size);
    void draw();
    void release();
    void enableVao();
    void diableVao();

public:
    GLuint buf = 0;
    GLuint vao = 0;
    GLuint texture = 0;
    unsigned char *image = nullptr;
    GLuint attriIndex[ATTRIB_COUNT] = {0};
    const char *attribNames[ATTRIB_COUNT] = {"av4position", "in_tex_coord"};
};

class textures : public Node
{
public:
    textures()
    {
        nodeName = "textures";
    }
protected:
    void cleanup();
    bool startup() override;
    void render(double currentTime, double difTime) override;
    vmath::mat4 createGLMatrix(float time);
    void shutdown() override;

protected:
    float maxAnisotropic;
    GLSLProgram program;

    struct uniforms_block {
        vmath::mat4 mv_matrix;
        vmath::mat4 view_matrix;
        vmath::mat4 proj_matrix;
    };

    GLuint uniforms_buffer;
    float camera_position;
    TextureWrapper textWrapper[4];
};

bool TextureWrapper::init(const char *filename, const GLfloat *data, int size)
{
    int width, height, bpp;
    image = stbi_load(filename, &width, &height, &bpp, STBI_default);

    if (image == 0 || width == 0 || height == 0 || bpp < 3 || bpp > 4) {
        return false;
    }

    GLenum format = bpp == 3 ? GL_RGB : GL_RGBA;

    GLint min_filter = GL_LINEAR;
    GLint mag_filter = GL_LINEAR;

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);

    if ((min_filter != GL_NEAREST && min_filter != GL_LINEAR) ||
            (mag_filter != GL_NEAREST && mag_filter != GL_LINEAR)) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    enableVao();
    return true;
}

void TextureWrapper::draw()
{
    glBindTexture(GL_TEXTURE_2D, texture);
    enableVao();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void TextureWrapper::release()
{
    diableVao();
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &buf);
    if (image) {
        stbi_image_free(image);
    }
}

void TextureWrapper::enableVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glVertexAttribPointer(attriIndex[0], 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attriIndex[0]);
    glVertexAttribPointer(attriIndex[1], 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)(16 * sizeof(float)));
    glEnableVertexAttribArray(attriIndex[1]);
}

void TextureWrapper::diableVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glDisableVertexAttribArray(attriIndex[0]);
    glDisableVertexAttribArray(attriIndex[1]);
}

void textures::cleanup()
{
    camera_position = 90.0f;
}

bool textures::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200) {
        return false;
    }

    bool ok = program.build("../media/shaders/texture/texture.vert",
                            "../media/shaders/texture/texture.frag");
    if (!ok) {
        return false;
    }
    for (int i = 0; i < 4; i++) {
        setAttribLocation(textWrapper[i].attriIndex, textWrapper[i].attribNames, program, ATTRIB_COUNT);
    }

    cleanup();

    //top
    const GLfloat vertices3[] = {
        -1, 1, 20, 1.0,
            1, 1, 20, 1.0,
            1, 1, -20, 1.0,
            -1, 1, -20, 1.0,
            0, 0,
            0, 1,
            20, 1,
            20, 0
        };

    textWrapper[0].init("../media/textures/texture/ceiling.png", vertices3, sizeof(vertices3));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //left
    const GLfloat vertices1[] = {
        -1, -1, 20, 1.0,
            -1, -1, -20, 1.0,
            -1, 1, -20, 1.0,
            -1, 1, 20, 1.0,
            0, 0,
            20, 0,
            20, 1,
            0, 1
        };

    textWrapper[1].init("../media/textures/texture/brick.png", vertices1, sizeof(vertices1));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //right
    const GLfloat vertices2[] = {
        1, -1, 20, 1.0,
        1, -1, -20, 1.0,
        1, 1, -20, 1.0,
        1, 1, 20, 1.0,
        0, 0,
        20, 0,
        20, 1,
        0, 1
    };

    textWrapper[2].init("../media/textures/texture/brick.png", vertices2, sizeof(vertices2));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //bottom
    const GLfloat vertices4[] = {
        -1, -1, 20, 1.0,
            1, -1, 20, 1.0,
            1, -1, -20, 1.0,
            -1, -1, -20, 1.0,
            0, 0,
            0, 1,
            20, 1,
            20, 0
        };

    textWrapper[3].init("../media/textures/texture/floor.png", vertices4, sizeof(vertices4));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glClearColor(0.0, 0.0, 0.0, 0.0);

    return true;
}

void textures::render(double currentTime, double difTime)
{
    (void)currentTime;

    glClear(GL_COLOR_BUFFER_BIT);

    program.use();

    auto matrix = createGLMatrix((float)difTime);
    program.setUniformMatrix4fv("mvp", matrix);

    textWrapper[0].draw();
    textWrapper[1].draw();

    textWrapper[2].draw();
    textWrapper[3].draw();
}

vmath::mat4 textures::createGLMatrix(float time)
{
    vmath::mat4 model_matrix = vmath::scale(14.0f);

    camera_position = camera_position - time * 5.0f;
    if (camera_position < -80.0f)
        camera_position = 90.0f;

    vmath::vec3 view_position = vmath::vec3(0, 0, camera_position);
    vmath::mat4 view_matrix = vmath::lookat(view_position,
                                            vmath::vec3(0.0f, 0.0f, 0.0f),
                                            vmath::vec3(0.0f, 1.0f, 0.0f));
    vmath::mat4 zoom_matrix = vmath::scale(6.00f, 6.00f, 6.00f);
    vmath::mat4 mv_matrix = view_matrix * model_matrix * zoom_matrix;
    vmath::mat4 proj_matrix = vmath::perspective(50.0f, getWindowRatio(), 0.1f, 1000.0f);
    return proj_matrix * mv_matrix;
}

void textures::shutdown()
{
    for (int i = 0; i < 4; i++)
        textWrapper[i].release();
    cleanup();
    program.clear();
}

REGISTER_OBJECT(Node, textures)
