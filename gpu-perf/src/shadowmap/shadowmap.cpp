#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <string.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "vmath.h"
#include "Node.h"
#include "ObjectFactory.h"
#include "GLSLProgram.h"

#define R_ATTRIB_COUNT 4
#define S_ATTRIB_COUNT 2

#define MAX(x,y) x>y? x:y
#define MIN(x,y) x>y? y:x
struct RenderObject {
    vmath::mat4 modelMatrix;
    std::vector<vmath::vec4> vertices;
    std::vector<vmath::vec4> normals;
    std::vector<vmath::vec4> colors;
};

std::vector<std::vector<vmath::vec4>> generateSphere(std::size_t slices, std::size_t stacks,
                                                     float radius)
{
    const double PI = 3.141592653589793;
    const double sliceStep = 360.0 / (double)slices;
    const double stackStep = 180.0 / (double)stacks;
    std::vector<std::vector<vmath::vec4>> result;
    std::vector<vmath::vec4> result_v;
    std::vector<vmath::vec4> result_r;
    auto degToRad = [PI](double deg) {
        return (PI / 180.0) * deg;
    };
    auto pointOf_v = [degToRad, radius](double theta, double phi) {
        return vmath::vec4{ radius *(float)std::cos(degToRad(theta)) *(float)std::cos(degToRad(phi)),
                            radius *(float)std::cos(degToRad(theta)) *(float)std::sin(degToRad(phi)),
                            radius *(float)std::sin(degToRad(theta)), 1.0 };
    };
    auto pointOf_r = [degToRad](double theta, double phi) {
        return vmath::vec4{ (float)std::cos(degToRad(theta)) *(float)std::cos(degToRad(phi)),  //
                            (float)std::cos(degToRad(theta)) *(float)std::sin(degToRad(phi)),  //
                            (float)std::sin(degToRad(theta)), 1.0 };
    };
    for (std::size_t stack = 0; stack < stacks; ++stack) {
        double theta = -90.0 + stack * stackStep;
        double thetaNext = -90.0 + (stack + 1) * stackStep;
        for (std::size_t slice = 0; slice < slices; ++slice) {
            double phi = slice * sliceStep;
            double phiNext = (slice + 1) * sliceStep;
            result_v.push_back(pointOf_v(theta, phi));
            result_v.push_back(pointOf_v(theta, phiNext));
            result_v.push_back(pointOf_v(thetaNext, phi));
            result_v.push_back(pointOf_v(thetaNext, phi));
            result_v.push_back(pointOf_v(theta, phiNext));
            result_v.push_back(pointOf_v(thetaNext, phiNext));

            result_r.push_back(vmath::normalize(pointOf_r(theta, phi)));
            result_r.push_back(vmath::normalize(pointOf_r(theta, phiNext)));
            result_r.push_back(vmath::normalize(pointOf_r(thetaNext, phi)));
            result_r.push_back(vmath::normalize(pointOf_r(thetaNext, phi)));
            result_r.push_back(vmath::normalize(pointOf_r(theta, phiNext)));
            result_r.push_back(vmath::normalize(pointOf_r(thetaNext, phiNext)));
        }
    }
    result.push_back(result_v);
    result.push_back(result_r);
    return result;
}

vmath::mat4 convertProjectionToImage(const vmath::mat4 &matrix)
{
    vmath::vec4 a(0.5, 0.0, 0.0, 0.0);
    vmath::vec4 b(0.0, 0.5, 0.0, 0.0);
    vmath::vec4 c(0.0, 0.0, 0.5, 0.0);
    vmath::vec4 d(0.5, 0.5, 0.5, 1.0);
    static const vmath::mat4 bias(a, b, c, d);
    return bias * matrix;
}

class shadowmap : public Node
{
public:
    shadowmap();
protected:
    bool startup()override;
    void render(double currentTime, double difTime)override;
    void shutdown()override;
    void onResized(int width, int height)override;
private:
    void shadowProgram(double currentTime, double difTime);
    void renderProgram(double currentTime, double difTime);

    void enableSVao();
    void enableRVao();
private:
    GLSLProgram _shadowProgram;
    GLSLProgram _renderProgram;
private:
    vmath::mat4 _renderMatrix;
    vmath::mat4 _shadowMatrix;
    vmath::mat4 _viewMatrix;
    vmath::mat4 _projectionMatrix;
    const float kFoV = 45.5f;
    const float kAspectRatio = 800.0f / 600.0f;
    const float kUpdateTimeFactor = 0.5f;
    const vmath::vec3 kCameraPosition = { 0.0f, 5.0, -15.0 };
    const vmath::vec3 kCameraTarget = { 0.0f, 0.0f, 0.0f };
    const vmath::vec2 kClipDistances = { 1.0f, 2000.0f };
    const vmath::vec3 kLightDirection = { 1.0f, -0.5f, 0.5f };
    const vmath::vec3 kLightPosition = vmath::normalize(-kLightDirection) * 5.0f;
    struct suniformdata {
        std::vector<vmath::mat4> mvp_data;
    };
    struct runiformdata {
        std::vector<vmath::mat4> mvp_data;
        std::vector<vmath::mat4> depthMVP_data;
    };
    std::vector<RenderObject> _renderObjects;
    suniformdata Uniformdata_s;
    runiformdata Uniformdata_r;
    GLuint _shadowmapFramebuffer;
    GLuint _shadowmapTexture;
    GLuint _colorTexture;
    GLuint all_vbo;

    GLuint rAttriIndex[R_ATTRIB_COUNT] = {0};
    const char *rAttribNames[R_ATTRIB_COUNT] = {"vertexPosition", "vertexNormal", "vertexColor", "vertexID"};
    GLuint sAttriIndex[S_ATTRIB_COUNT] = {0};
    const char *sAttribNames[S_ATTRIB_COUNT] = {"vertexPosition", "vertexID"};
};

shadowmap::shadowmap()
{
    nodeName = "shadowmap";
}

void shadowmap::onResized(int width, int height)
{
    glViewport(0, 0, width, height);
    _projectionMatrix = vmath::perspective(45.0, width / float(height), kClipDistances[0],
                                           kClipDistances[1]);
    _viewMatrix = vmath::lookat(kCameraPosition, kCameraTarget, vmath::vec3{ 0.0f, 1.0f, 0.0f });
    Node::onResized(width, height);
}

bool shadowmap::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200) {
        return false;
    }
    bool ok = _shadowProgram.build("../media/shaders/shadowmap/shadow.vert",
                                   "../media/shaders/shadowmap/shadow.frag");
    if (!ok) {
        return false;
    }
    ok = _renderProgram.build("../media/shaders/shadowmap/render.vert",
                              "../media/shaders/shadowmap/render.frag");
    if (!ok) {
        return false;
    }
    setAttribLocation(sAttriIndex, sAttribNames, _shadowProgram, S_ATTRIB_COUNT);
    setAttribLocation(rAttriIndex, rAttribNames, _renderProgram, R_ATTRIB_COUNT);

    _projectionMatrix = vmath::perspective(kFoV, kAspectRatio, kClipDistances[0], kClipDistances[1]);
    _viewMatrix = vmath::lookat(kCameraPosition, kCameraTarget, vmath::vec3{ 0.0f, 1.0f, 0.0f });
    _renderMatrix = _projectionMatrix * _viewMatrix;
    vmath::mat4 shadowProjectionMatrix = vmath::ortho(-30.0f, 30.0f, -15.0f, 15.0f, -30.0f, 30.0f);
    vmath::mat4 shadowViewMatrix = vmath::lookat(kLightPosition, vmath::vec3{ 0.0f }, vmath::vec3{ 0.0f, 1.0f, 0.0f });
    _shadowMatrix = shadowProjectionMatrix * shadowViewMatrix;

    std::vector<vmath::vec4> normals = std::vector<vmath::vec4>(36);
    const std::vector<vmath::vec4> vertices = {
        { -0.5f, -0.5f, -0.5f, 1.0f }, { -0.5f, -0.5f, 0.5f, 1.0f }, { -0.5f, 0.5f, 0.5f, 1.0f }, { 0.5f, 0.5f, -0.5f, 1.0f }, { -0.5f, -0.5f, -0.5f, 1.0f }, { -0.5f, 0.5f, -0.5f, 1.0f },
        { 0.5f, -0.5f, 0.5f, 1.0f }, { -0.5f, -0.5f, -0.5f, 1.0f }, { 0.5f, -0.5f, -0.5f, 1.0f }, { 0.5f, 0.5f, -0.5f, 1.0f }, { 0.5f, -0.5f, -0.5f, 1.0f }, { -0.5f, -0.5f, -0.5f, 1.0f },
        { -0.5f, -0.5f, -0.5f, 1.0f }, { -0.5f, 0.5f, 0.5f, 1.0f }, { -0.5f, 0.5f, -0.5f, 1.0f }, { 0.5f, -0.5f, 0.5f, 1.0f }, { -0.5f, -0.5f, 0.5f, 1.0f }, { -0.5f, -0.5f, -0.5f, 1.0f },
        { -0.5f, 0.5f, 0.5f, 1.0f }, { -0.5f, -0.5f, 0.5f, 1.0f }, { 0.5f, -0.5f, 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, { 0.5f, -0.5f, -0.5f, 1.0f }, { 0.5f, 0.5f, -0.5f, 1.0f },
        { 0.5f, -0.5f, -0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, { 0.5f, -0.5f, 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, { 0.5f, 0.5f, -0.5f, 1.0f }, { -0.5f, 0.5f, -0.5f, 1.0f },
        { 0.5f, 0.5f, 0.5f, 1.0f }, { -0.5f, 0.5f, -0.5f, 1.0f }, { -0.5f, 0.5f, 0.5f, 1.0f }, { 0.5f, 0.5f, 0.5f, 1.0f }, { -0.5f, 0.5f, 0.5f, 1.0f }, { 0.5f, -0.5f, 0.5f, 1.0f },
    };
    for (int x = -1; x < 1; ++x) {
        for (int y = -1; y < 1; ++y) {
            vmath::mat4 cubeModelMatrix;
            vmath::vec4 cubeColor = { 0.4f, 0.4f, 0.4f, 1.0f };
            if ((x + y) % 2) {
                cubeColor += vmath::vec4{ 0.1f, 0.1f, 0.1f, 0.0f };
            }
            vmath::mat4 scale_matrix = vmath::scale(6.0f, 1.0f, 6.0f);
            vmath::mat4 translate_matrix = vmath::translate((float)x + 1.5f, -1.5f, (float)y + 0.5f);
            cubeModelMatrix = scale_matrix * translate_matrix;
            _renderObjects.push_back({ cubeModelMatrix, vertices, normals, std::vector<vmath::vec4>(vertices.size(), cubeColor) });
        }
    }
    for (int x = 0; x < 1; ++x) {
        for (int y = 0; y < 1; ++y) {
            vmath::mat4 cubeModelMatrix;
            vmath::vec4 cubeColor = { 0.25f, 0.25f, 0.25f, 1.0f };
            if (y % 2 == 0) {
                cubeColor = { 0.25f, 0.25f, 0.25f, 1.0f };
            } else {
                cubeColor = { 0.75f, 0.75f, 0.75f, 1.0f };
            }
            cubeModelMatrix = vmath::translate((float)(2.0f * x), 0.7f, (float)(2.0f * y));
            if ((x + y) % 2) {
                vmath::mat4 translate_matrix = vmath::translate<float>(0.0, 1.0, 0.0);
                cubeModelMatrix = cubeModelMatrix * translate_matrix;
            }
            _renderObjects.push_back(
            { cubeModelMatrix, vertices, normals, std::vector<vmath::vec4>(vertices.size(), cubeColor) });
        }
    }
    std::vector<std::vector<vmath::vec4>> sphereGenerator_data = generateSphere(30, 30, 1.0f);
    vmath::mat4 sphereModelMatrix1, sphereModelMatrix2, sphereModelMatrix3, sphereModelMatrix4;
    vmath::vec4 sphereColor = { 0.2f, 0.2f, 0.2f, 1.0f };

    sphereModelMatrix1 = vmath::translate(4.0f, 3.5f, 0.0f);
    sphereModelMatrix2 = vmath::translate(-4.0f, 3.5f, 0.0f);
    sphereModelMatrix3 = vmath::translate(0.0f, 3.5f, 4.0f);
    sphereModelMatrix4 = vmath::translate(0.0f, 3.5f, -4.0f);

    sphereModelMatrix1 = sphereModelMatrix1 * vmath::scale(1.35f, 1.35f, 1.35f);
    sphereModelMatrix2 = sphereModelMatrix2 * vmath::scale(1.35f, 1.35f, 1.35f);
    sphereModelMatrix3 = sphereModelMatrix3 * vmath::scale(1.35f, 1.35f, 1.35f);
    sphereModelMatrix4 = sphereModelMatrix4 * vmath::scale(1.35f, 1.35f, 1.35f);
    /*_renderObjects.push_back({ sphereModelMatrix1, sphereGenerator_data[0], sphereGenerator_data[1],
                               std::vector<vmath::vec4>(sphereGenerator_data[0].size(), sphereColor) });
    _renderObjects.push_back({ sphereModelMatrix2, sphereGenerator_data[0], sphereGenerator_data[1],
                               std::vector<vmath::vec4>(sphereGenerator_data[0].size(), sphereColor) });*/
    /*_renderObjects.push_back({ sphereModelMatrix3, sphereGenerator_data[0], sphereGenerator_data[1],
                               std::vector<vmath::vec4>(sphereGenerator_data[0].size(), sphereColor) });*/
    /*_renderObjects.push_back({ sphereModelMatrix4, sphereGenerator_data[0], sphereGenerator_data[1],
                               std::vector<vmath::vec4>(sphereGenerator_data[0].size(), sphereColor) });*/
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glGenTextures(1, &_colorTexture);
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 600, 600, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &_shadowmapTexture);
    glBindTexture(GL_TEXTURE_2D, _shadowmapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 600, 600, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &_shadowmapFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _shadowmapFramebuffer);
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTexture, 0);
    glBindTexture(GL_TEXTURE_2D, _shadowmapTexture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowmapTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<vmath::vec4> vbo_data;
    int vertexID = 0;
    for (std::size_t index = 0; index < _renderObjects.size(); ++index) {
        for (std::size_t index1 = 0; index1 < _renderObjects[index].vertices.size(); ++index1) {
            vbo_data.push_back(_renderObjects[index].vertices[index1]);
            vbo_data.push_back(_renderObjects[index].normals[index1]);
            vbo_data.push_back(_renderObjects[index].colors[index1]);
            vbo_data.push_back(vmath::vec4(vertexID++));
        }
    }
    GLvoid *v_data = (GLvoid *)(vbo_data.data());
    GLsizeiptr size = sizeof(vmath::vec4) * vbo_data.size();
    glGenBuffers(1, &all_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, all_vbo);
    glBufferData(GL_ARRAY_BUFFER, size, v_data, GL_STATIC_DRAW);

    for (const auto &renderObject : _renderObjects) {
        vmath::mat4 MVP_s = _shadowMatrix * renderObject.modelMatrix;
        vmath::mat4 MVP_r = _renderMatrix * renderObject.modelMatrix;
        vmath::mat4 depthMVP_r = convertProjectionToImage(_shadowMatrix * renderObject.modelMatrix);
        Uniformdata_s.mvp_data.push_back(MVP_s);
        Uniformdata_r.mvp_data.push_back(MVP_r);
        Uniformdata_r.depthMVP_data.push_back(depthMVP_r);
    }

    for (size_t i = 0; i < Uniformdata_s.mvp_data.size(); i++) {
        _shadowProgram.use();
        _shadowProgram.setUniformMatrix4fv(std::string("MVP[" + std::to_string(i) + "]").c_str(),
                                           Uniformdata_s.mvp_data[i]);
    }

    return true;
}

void shadowmap::shadowProgram(double currentTime, double difTime)
{
    glBindFramebuffer(GL_FRAMEBUFFER, _shadowmapFramebuffer);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, 600, 600);
    glClear(GL_DEPTH_BUFFER_BIT);
    _shadowProgram.use();
    enableSVao();
    _shadowProgram.setUniform1i("prime_offset", 0);
    _shadowProgram.setUniform1i("prime_vertexcnt", 36);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(_renderObjects.size()) * 36);
    /*_shadowProgram.setUniform1i("prime_offset", 288);
    _shadowProgram.setUniform1i("prime_vertexcnt", 5400);
    glDrawArrays(GL_TRIANGLES, 8 * 36, (GLsizei)1 * 5400);*/
}

void shadowmap::renderProgram(double currentTime, double difTime)
{
    double time = PerfWindow::perfGetTime() * kUpdateTimeFactor;
    _viewMatrix = vmath::lookat(vmath::vec3{ (float)(std::cos(time) * 10.0), 8.0, (float)(std::sin(time) * 10.0) },
                                kCameraTarget,
                                vmath::vec3{ 0.0f, 1.0f, 0.0f });
    _renderMatrix = _projectionMatrix * _viewMatrix;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, getWidth(), getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    _renderProgram.use();

    for (std::size_t index2 = 0; index2 < _renderObjects.size(); ++index2) {
        vmath::mat4 MVP_r = _renderMatrix * _renderObjects[index2].modelMatrix;
        vmath::mat4 depthMVP_r = convertProjectionToImage(_shadowMatrix *
                                                          _renderObjects[index2].modelMatrix);
        Uniformdata_r.mvp_data[index2] = MVP_r;
        Uniformdata_r.depthMVP_data[index2] = depthMVP_r;
    }

    for (size_t i = 0; i < Uniformdata_r.mvp_data.size(); i++) {
        _renderProgram.setUniformMatrix4fv(std::string("MVP[" + std::to_string(i) + "]").c_str(),
                                           Uniformdata_r.mvp_data[i]);

    }
    for (size_t i = 0; i < Uniformdata_r.depthMVP_data.size(); i++) {
        _renderProgram.setUniformMatrix4fv(std::string("depthMVP[" + std::to_string(i) + "]").c_str(),
                                           Uniformdata_r.depthMVP_data[i]);
    }

    enableRVao();
    _renderProgram.setUniform1i("prime_offset", 0);
    _renderProgram.setUniform1i("prime_vertexcnt", 36);
    glDrawArrays(GL_TRIANGLES, 0, ((GLint)_renderObjects.size()) * 36);
    /*_renderProgram.setUniform1i("prime_offset", 288);
    _renderProgram.setUniform1i("prime_vertexcnt", 5400);
    glDrawArrays(GL_TRIANGLES, 8 * 36, (GLint)1 * 5400);*/
}

void shadowmap::render(double currentTime, double difTime)
{
    (void)difTime;
    glEnable(GL_DEPTH_TEST);
    shadowProgram(currentTime, difTime);
    renderProgram(currentTime, difTime);
    glDisable(GL_DEPTH_TEST);

}

void shadowmap::shutdown()
{
    glDeleteFramebuffers(1, &_shadowmapFramebuffer);
    glDeleteBuffers(1, &all_vbo);
    glDeleteTextures(1, &_shadowmapTexture);
    glDeleteTextures(1, &_colorTexture);
    _shadowProgram.clear();
    _renderProgram.clear();

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
}

void shadowmap::enableSVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, all_vbo);
    glEnableVertexAttribArray(sAttriIndex[0]);
    glVertexAttribPointer(sAttriIndex[0], 4, GL_FLOAT, GL_FALSE, 4 * sizeof(vmath::vec4), nullptr);
    glEnableVertexAttribArray(sAttriIndex[1]);
    glVertexAttribPointer(sAttriIndex[1], 4, GL_FLOAT, GL_FALSE, 4 * sizeof(vmath::vec4),
                          (const void *)(3 * sizeof(vmath::vec4)));
}

void shadowmap::enableRVao()
{
    glBindBuffer(GL_ARRAY_BUFFER, all_vbo);
    glEnableVertexAttribArray(rAttriIndex[0]);
    glVertexAttribPointer(rAttriIndex[0], 4, GL_FLOAT, GL_FALSE, 4 * sizeof(vmath::vec4), nullptr);
    glEnableVertexAttribArray(rAttriIndex[2]);
    glVertexAttribPointer(rAttriIndex[2], 4, GL_FLOAT, GL_FALSE, 4 * sizeof(vmath::vec4),
                          (const void *)(2 * sizeof(vmath::vec4)));
    glEnableVertexAttribArray(rAttriIndex[3]);
    glVertexAttribPointer(rAttriIndex[3], 4, GL_FLOAT, GL_FALSE, 4 * sizeof(vmath::vec4),
                          (const void *)(3 * sizeof(vmath::vec4)));

}
REGISTER_OBJECT(Node, shadowmap)
