/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    vertexrate.cpp
*  @brif:    vertexrate cpp
*
*  @date:    09/02/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <string>
#include <cstdlib>
#include <vector>
#include <array>
#include <random>
#include <memory>
#include <vmath.h>
#include <glad/glad.h>
#include <GLSLProgram.h>
#include <TextRender.h>
#include <Node.h>
#include <ObjectFactory.h>
#include <stb_image.h>

#define RECT_NUMBER 8
#define VERTEX_COMPONENT_NUM 3
#define COLOR_COMPONENT_NUM 4
#define MAX_COLOR_VALUE 255
#define MAX_ANGLE_VALUE 360
#define ROTATE_STAY_TIME 10
#define ROTATE_SPEED 2
#define DRAW_COUNT 1

#define BASE_SIZE = 72;
#define MIN_SIZE = 32;
#define MAX_SIZE = 64;

typedef struct _ImagePiexlComponent
{
    GLint x;
    GLint y;
    GLubyte r;
    GLubyte g;
    GLubyte b;
    GLubyte a;
    GLuint rgbType;
    _ImagePiexlComponent()
    {
        x = 0;
        y = 0;
        r = 0;
        g = 0;
        b = 0;
        a = 0;
        rgbType = GL_RGB;
    }
}ImagePiexlComponent;
typedef  std::vector<std::vector<ImagePiexlComponent>> ImgPiexlArray;

enum VERTEX_OPERATION_TYPE{
    EARTHQUAKE = 0,
    ROTATE_Y,
    ROTATE_Z,
};

enum RENDER_TYPE{
    RENDER_ONE = 0,
    RENDER_TOW,
};

class pixel: public Node
{
public:
    pixel();
protected:
    NodeType getNodeType();

    bool startup();
    void render(double currentTime, double difTime);
    void shutdown();

    void loadImageToPiexlArray(std::string imgFile,\
                           ImgPiexlArray& vertexDataArray);
    void convertImgPiexlToVertices(std::vector<GLfloat>& vertexArray,\
                                   std::vector<GLfloat>& vertexColorArray,\
                                   ImgPiexlArray& vertexDataArray);
    void loadVertexData();
    bool loadProgram();
    void startTransform();
private:
    void initializeVertexData(void);
    void enableVertex();
private:
    GLSLProgram program;
    GLuint vbo[2];
    std::vector<float> vertexData;
    std::vector<float> colorData;

    unsigned int screenWidth ;
    unsigned int screenHeight;
    unsigned int drawWidth ;
    unsigned int drawHeight;
    ImgPiexlArray imgPiexl;
    GLint renderType;

    vmath::mat4 groundTransform;

    GLuint radiusX;
    GLfloat radiusXF;
    GLuint radiusY ;
    GLfloat radiusYF ;
    GLfloat rotateAngle;
    GLint rotate180StopTime;
    GLint rotateStep;
    GLint rotateCount;
    GLint operationType;
};

pixel::pixel()
{
    nodeName = "pixel";

    renderType = RENDER_TYPE::RENDER_TOW;
    groundTransform = vmath::mat4(1.0f);
    radiusX = 0;
    radiusXF = 0;
    radiusY = 0;
    radiusYF = 0;
    rotateAngle = 0;
    operationType = VERTEX_OPERATION_TYPE::ROTATE_Z;
    rotate180StopTime = 0;
    rotateStep = ROTATE_SPEED;
    rotateCount = 0;
    loadImageToPiexlArray("../media/textures/ground03.jpeg", imgPiexl);
}

Node::NodeType pixel::getNodeType()
{
    return NodeType_FillRate;
}

bool pixel::startup()
{
    if (PerfWindow::get()->getGLESVersion() < 200)
        return false;

    if(!loadProgram())
    {
        return false;
    }
    initializeVertexData();

    return true;
}

void pixel::render(double currentTime, double difTime)
{
   (void)currentTime;
   (void)difTime;
   startTransform();
}

void pixel::shutdown()
{
    glDeleteBuffers(2, vbo);
    program.clear();
    vertexData.clear();
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void pixel::initializeVertexData(void)
{
    screenWidth  = static_cast<GLuint>(getWidth());
    screenHeight = static_cast<GLuint>(getHeight());
    drawWidth = screenWidth / 2;
    drawHeight = screenHeight / 2;
    radiusX = ((drawWidth)/RECT_NUMBER) / 2;
    radiusXF = static_cast<GLfloat>(radiusX)/ \
               static_cast<GLfloat>(drawWidth);
    radiusY = ((drawHeight)/RECT_NUMBER) / 2;
    radiusYF = static_cast<GLfloat>(radiusY)/ \
               static_cast<GLfloat>(drawHeight);
    program.use();
    program.setUniform1f("radiusX", radiusXF*2);
    program.setUniform1f("radiusY", radiusYF*2);
    program.setUniform1i("rectNum", RECT_NUMBER);
    program.setUniform1i("operationEnum", operationType);

    loadVertexData();
}

void pixel::loadVertexData()
{
    convertImgPiexlToVertices(vertexData, colorData, imgPiexl);
    glGenBuffers(2, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    gl_BufferData(GL_ARRAY_BUFFER, vertexData, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    gl_BufferData(GL_ARRAY_BUFFER, colorData, GL_STATIC_DRAW);
    enableVertex();
}

void pixel::enableVertex()
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, nullptr );
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr );
    glEnableVertexAttribArray(1);
}

bool pixel::loadProgram()
{
    bool ok = program.build("../media/shaders/vertexrate/vs.glsl",\
                  "../media/shaders/vertexrate/fs.glsl");
    if(!ok)
    {
        return ok;
    }
    program.use();
    return ok;
}

void pixel::startTransform()
{
    glClear(GL_COLOR_BUFFER_BIT);

    program.use();

    if(rotate180StopTime <= 0)
    {
        rotateAngle+=rotateStep;
        if(rotateAngle >= MAX_ANGLE_VALUE)
        {
            rotateCount++;
            rotateStep = -ROTATE_SPEED;
        }
        if(rotateAngle <= 0)
        {
            rotateStep = ROTATE_SPEED;
        }
        program.setUniform1i("operationEnum", operationType);
        program.setUniform1f("rotateAngle",static_cast<GLfloat>(vmath::radians(rotateAngle) ));
    }

    if( (static_cast<GLint>(rotateAngle) == (MAX_ANGLE_VALUE / 2) \
         || (static_cast<GLint>(rotateAngle) == MAX_ANGLE_VALUE) \
         || (static_cast<GLint>(rotateAngle) == 0) )\
            && rotate180StopTime <= 0)
    {
        rotate180StopTime = ROTATE_STAY_TIME;
    }
    if(rotate180StopTime > 0)
    {
        rotate180StopTime--;
    }

    enableVertex();
    for (int i = 0; i < DRAW_COUNT; i++) {
        glDrawArrays(GL_POINTS, 0, static_cast<GLint>(vertexData.size()/VERTEX_COMPONENT_NUM));
    }
}

void pixel::loadImageToPiexlArray(std::string imgFile,\
                       ImgPiexlArray& imgPiexlDataArray)
{
    GLint width = 0;
    GLint height = 0;
    GLint nrChannels = 0;
    GLubyte* data = stbi_load(imgFile.data(), &width, &height, &nrChannels, 0);
    imgPiexlDataArray.clear();
    imgPiexlDataArray.resize(static_cast<GLuint>(height));
    for (GLint row = 0; row < height; row++)
    {
        std::vector<ImagePiexlComponent> pixelArray(static_cast<GLuint>(width));
        for (GLint col = 0; col < width; col++)
        {

            GLubyte *dataOffset = data + ((col + row * width) * nrChannels);
            ImagePiexlComponent pixel;
            pixel.x = col;
            pixel.y = row;
            pixel.r = dataOffset[0];
            pixel.g = dataOffset[1];
            pixel.b = dataOffset[2];
            pixel.a = nrChannels >= 4 ? dataOffset[3] : 0xff;
            pixel.rgbType = nrChannels >= 4 ? GL_RGBA : GL_RGB;
            pixelArray[static_cast<GLuint>(col)] = pixel;
        }
        imgPiexlDataArray[static_cast<GLuint>(row)] = pixelArray;
    }
    stbi_image_free(data);
}

void pixel::convertImgPiexlToVertices(std::vector<GLfloat>& vertexArray,\
                               std::vector<GLfloat>& vertexColorArray,\
                               ImgPiexlArray& imgPixelArray)
{
    GLuint64 imgHeight =imgPixelArray.size() - 60;
    GLuint64 imgWidth = imgPixelArray[0].size();
    vertexArray.clear();
    vertexColorArray.clear();
    vertexArray.resize(drawHeight * drawWidth * VERTEX_COMPONENT_NUM);
    vertexColorArray.resize(drawHeight * drawWidth * COLOR_COMPONENT_NUM);

    GLfloat screenAndImgRateW = static_cast<GLfloat>(imgWidth)/ \
                                 static_cast<GLfloat>(drawWidth);
    GLfloat screenAndImgRateH = static_cast<GLfloat>(imgHeight)/ \
                                 static_cast<GLfloat>(drawHeight);
    for (GLuint64 row = 0; row < drawHeight; row++)
    {
        for (GLuint64 col = 0; col < drawWidth; col++)
        {
            GLuint convertedX = static_cast<GLuint>(round(screenAndImgRateW * \
                                 static_cast<GLfloat>(col) ));
            GLuint convertedY = static_cast<GLuint>(round(screenAndImgRateH * \
                                 static_cast<GLfloat>(row) ));
            ImagePiexlComponent tempPixel = imgPixelArray[convertedY][convertedX];
            tempPixel.x = static_cast<GLint>(col);
            tempPixel.y = static_cast<GLint>(row);

            GLuint64 pixelIndexOffset = (col + row * drawWidth) * VERTEX_COMPONENT_NUM;
            vertexArray[pixelIndexOffset] = static_cast<GLfloat>(tempPixel.x + 200 \
                                              - static_cast<GLint>(screenWidth/2))\
                                                 / static_cast<GLfloat>(screenWidth/2);

            vertexArray[pixelIndexOffset + 1] = static_cast<GLfloat>(static_cast<GLint>(screenHeight/2) \
                                                  - (tempPixel.y + 150)) / static_cast<GLfloat>(screenHeight/2);
            vertexArray[pixelIndexOffset + 2] = 0.0f;

            GLuint64 colorIndexOffset = (col + row * drawWidth) * COLOR_COMPONENT_NUM;
            vertexColorArray[colorIndexOffset] = static_cast<GLfloat>(tempPixel.r) / MAX_COLOR_VALUE;
            vertexColorArray[colorIndexOffset + 1] = static_cast<GLfloat>(tempPixel.g) / MAX_COLOR_VALUE;
            vertexColorArray[colorIndexOffset + 2] = static_cast<GLfloat>(tempPixel.b) / MAX_COLOR_VALUE;
            vertexColorArray[colorIndexOffset + 3] = static_cast<GLfloat>(tempPixel.a) / MAX_COLOR_VALUE;
        }
    }
}

REGISTER_OBJECT(Node, pixel)
