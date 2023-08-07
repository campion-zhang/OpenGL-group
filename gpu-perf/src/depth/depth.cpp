/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    depth.cpp
*  @brif:    depth test
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include "vmath.h"
#include "Texture.h"
#include "Node.h"
#include "ObjectFactory.h"
#include "vmath.h"
#define CUBE_ATTRIB_COUNT 2
#define GLASS_ATTRIB_COUNT 1
class depth : public Node
{
public:
    depth();
protected:
    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();
private:
    void renderCube(double currentTime, double difTime);
    void renderCircle(double currentTime, double difTime);
    void enableVao1();
    void enableVao2();
private:
    GLSLProgram programCube;
    GLSLProgram programCircle;

    float verticesCube[108] =
    {
        /* Front face. */
        /* Bottom left */
        -0.5,  0.5, -0.5,
            0.5, -0.5, -0.5,
            -0.5, -0.5, -0.5,
            /* Top right */
            -0.5,  0.5, -0.5,
            0.5,  0.5, -0.5,
            0.5, -0.5, -0.5,
            /* Left face */
            /* Bottom left */
            -0.5,  0.5,  0.5,
            -0.5, -0.5, -0.5,
            -0.5, -0.5,  0.5,
            /* Top right */
            -0.5,  0.5,  0.5,
            -0.5,  0.5, -0.5,
            -0.5, -0.5, -0.5,
            /* Top face */
            /* Bottom left */
            -0.5,  0.5,  0.5,
            0.5,  0.5, -0.5,
            -0.5,  0.5, -0.5,
            /* Top right */
            -0.5,  0.5,  0.5,
            0.5,  0.5,  0.5,
            0.5,  0.5, -0.5,
            /* Right face */
            /* Bottom left */
            0.5,  0.5, -0.5,
            0.5, -0.5,  0.5,
            0.5, -0.5, -0.5,
            /* Top right */
            0.5,  0.5, -0.5,
            0.5,  0.5,  0.5,
            0.5, -0.5,  0.5,
            /* Back face */
            /* Bottom left */
            0.5,  0.5,  0.5,
            -0.5, -0.5,  0.5,
            0.5, -0.5,  0.5,
            /* Top right */
            0.5,  0.5,  0.5,
            -0.5,  0.5,  0.5,
            -0.5, -0.5,  0.5,
            /* Bottom face */
            /* Bottom left */
            -0.5, -0.5, -0.5,
            0.5, -0.5,  0.5,
            -0.5, -0.5,  0.5,
            /* Top right */
            -0.5, -0.5, -0.5,
            0.5, -0.5, -0.5,
            0.5, -0.5,  0.5,
        };

    const float colorsCube[108] =
    {
        /* Front face */
        /* Bottom left */
        1.0, 0.0, 0.0, /* red */
        1.0, 0.0, 0.0, /* red */
        1.0, 0.0, 0.0, /* red */
        /* Top right */
        1.0, 0.0, 0.0, /* red */
        1.0, 0.0, 0.0, /* red */
        1.0, 0.0, 0.0, /* red */

        /* Left face */
        /* Bottom left */
        0.0, 0.0, 1.0, /* blue */
        0.0, 0.0, 1.0, /* blue */
        0.0, 0.0, 1.0, /* blue */
        /* Top right */
        0.0, 0.0, 1.0, /* blue */
        0.0, 0.0, 1.0, /* blue */
        0.0, 0.0, 1.0, /* blue */

        /* Top face */
        /* Bottom left */
        1.0, 1.0, 1.0, /* white */
        1.0, 1.0, 1.0, /* white */
        1.0, 1.0, 1.0, /* white */
        /* Top right */
        1.0, 1.0, 1.0, /* white */
        1.0, 1.0, 1.0, /* white */
        1.0, 1.0, 1.0, /* white */

        /* Right face */
        /* Bottom left */
        1.0, 1.0, 0.0, /* yellow */
        1.0, 1.0, 0.0, /* yellow */
        1.0, 1.0, 0.0, /* yellow */
        /* Top right */
        1.0, 1.0, 0.0, /* yellow */
        1.0, 1.0, 0.0, /* yellow */
        1.0, 1.0, 0.0, /* yellow */

        /* Back face */
        /* Bottom left */
        0.0, 1.0, 1.0, /* cyan */
        0.0, 1.0, 1.0, /* cyan */
        0.0, 1.0, 1.0, /* cyan */
        /* Top right */
        0.0, 1.0, 1.0, /* cyan */
        0.0, 1.0, 1.0, /* cyan */
        0.0, 1.0, 1.0, /* cyan */

        /* Bottom face */
        /* Bottom left */
        1.0, 0.0, 1.0, /* magenta */
        1.0, 0.0, 1.0, /* magenta */
        1.0, 0.0, 1.0, /* magenta */
        /* Top right */
        1.0, 0.0, 1.0, /* magenta */
        1.0, 0.0, 1.0, /* magenta */
        1.0, 0.0, 1.0, /* magenta */
    };

    std::vector<GLfloat> verticesMirror;

    GLuint fbo = 0;
    GLuint buffer1 = 0;
    GLuint buffer2 = 0;
    GLuint texture = 0;
    GLuint colorTexture = 0;

    GLuint cubeAttriIndex[CUBE_ATTRIB_COUNT] = {0};
    const char *cubeAttribNames[CUBE_ATTRIB_COUNT] = {"av4position", "av3color"};
    GLuint glassAttriIndex[GLASS_ATTRIB_COUNT] = {0};
    const char *glassAttribNames[GLASS_ATTRIB_COUNT] = {"position"};
};

depth::depth()
{
    nodeName = "depth";
}

bool depth::startup()
{
    if(PerfWindow::get()->getGLESVersion() < 200)
        return false;

    bool ok = programCube.build("../media/shaders/depth/depth_cube.vert", "../media/shaders/depth/depth_cube.frag");
    if(!ok)
        return false;
    setAttribLocation(cubeAttriIndex, cubeAttribNames, programCube, CUBE_ATTRIB_COUNT);

    ok = programCircle.build("../media/shaders/depth/depth_glass.vert", "../media/shaders/depth/depth_glass.frag");
    if(!ok)
        return false;
    setAttribLocation(glassAttriIndex, glassAttribNames, programCircle, GLASS_ATTRIB_COUNT);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTexture);
    /* Texture unit 1 for depth buffer */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getWidth(), getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    glGenTextures(1, &texture);
    /* Texture unit 1 for depth buffer */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, getWidth(), getHeight(), 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);

    /*if(GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
        printf("glCheckFramebufferStatue failed\n");*/

    glGenBuffers(1, &buffer1);
    glBindBuffer(GL_ARRAY_BUFFER, buffer1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCube) + sizeof(colorsCube), nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verticesCube), verticesCube);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(verticesCube), sizeof(colorsCube), colorsCube);
    glEnableVertexAttribArray(cubeAttriIndex[0]);
    glVertexAttribPointer(cubeAttriIndex[0], 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(cubeAttriIndex[1]);
    glVertexAttribPointer(cubeAttriIndex[1], 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(verticesCube)));

    const int COUNT = 1200;
    const float degree = 2 * 3.14156265f / COUNT;

    verticesMirror.resize(COUNT * 3);
    verticesMirror[0] = 0.f;
    verticesMirror[1] = 0.f;
    verticesMirror[2] = 1.0f;
    for(int i = 0; i < COUNT; i++)
    {
        verticesMirror[static_cast<GLuint>(i * 3 + 0)] = 0.5f * cos(degree * i);
        verticesMirror[static_cast<GLuint>(i * 3 + 1)] = 0.5f * sin(degree * i);
        verticesMirror[static_cast<GLuint>(i * 3 + 2)] = 1.0f;
    }

    glGenBuffers(1, &buffer2);
    glBindBuffer(GL_ARRAY_BUFFER, buffer2);
    gl_BufferData(GL_ARRAY_BUFFER, verticesMirror, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glassAttriIndex[0]);
    glVertexAttribPointer(glassAttriIndex[0], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    return true;
}

void depth::shutdown()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteBuffers(1, &buffer1);
    glDeleteBuffers(1, &buffer2);
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &colorTexture);

    programCube.clear();
    programCircle.clear();

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBlendFunc(GL_ONE, GL_ZERO);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void depth::render(double currentTime, double difTime)
{
    (void)difTime;

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    /* Should do: src * (src alpha) + dest * (1-src alpha). */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.6f, 0.7f, 0.8f, 1.0f);

    renderCube(currentTime, difTime);
    renderCircle(currentTime, difTime);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void depth::renderCube(double currentTime, double difTime)
{
    (void)difTime;
    programCube.use();
    enableVao1();

    float f = static_cast<float>(currentTime * 4.0);

    vmath::mat4 model_matrix = vmath::scale(7.0f);
    vmath::vec3 view_position = vmath::vec3(cosf(f * 0.35f) * 120.0f, 45.0f, sinf(f * 0.35f) * 120.0f);
    vmath::mat4 view_matrix = vmath::lookat(view_position,
                                            vmath::vec3(0.0f, 0.0f, 0.0f),
                                            vmath::vec3(0.0f, 1.0f, 0.0f));
    vmath::mat4 zoom_matrix = vmath::scale(6.00f, 6.00f, 6.00f);
    vmath::mat4 mv_matrix = view_matrix * model_matrix * zoom_matrix;
    vmath::mat4 proj_matrix = vmath::perspective(48.0f,
                              getWindowRatio(),
                              0.1f,
                              1000.0f);

    programCube.setUniformMatrix4fv("mvp", (proj_matrix * mv_matrix).operator float * ());

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void depth::renderCircle(double currentTime, double difTime)
{
    (void)currentTime;
    (void)difTime;
    glDisable(GL_DEPTH_TEST);

    programCircle.use();
    enableVao2();

    const float near = -0.1f;
    const float far = 100.0f;
    const float r = getWindowRatio();
    const float k = 1.2f;
    auto perspective = vmath::ortho(-r * k, r * k, -k, k, near, far);

    programCircle.setUniform1f("near", near);
    programCircle.setUniform1f("far", far);
    programCircle.setUniformMatrix4fv("mvp", perspective.operator float * ());
    programCircle.setUniform1f("glassMaterial", 0.1f);
    programCircle.setUniform2f("viewportSize", getWidth(), getHeight());
    programCircle.setUniform1i("cubeDepthTexture", 1);

    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLint>(verticesMirror.size() / 3));
}

void depth::enableVao1()
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCube) + sizeof(colorsCube), nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verticesCube), verticesCube);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(verticesCube), sizeof(colorsCube), colorsCube);
    glEnableVertexAttribArray(cubeAttriIndex[0]);
    glVertexAttribPointer(cubeAttriIndex[0], 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(cubeAttriIndex[1]);
    glVertexAttribPointer(cubeAttriIndex[1], 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(verticesCube)));
}

void depth::enableVao2()
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer2);
    gl_BufferData(GL_ARRAY_BUFFER, verticesMirror, GL_STATIC_DRAW);
    glEnableVertexAttribArray(glassAttriIndex[0]);
    glVertexAttribPointer(glassAttriIndex[0], 3, GL_FLOAT, GL_FALSE, 0, nullptr);
}
REGISTER_OBJECT(Node, depth)
