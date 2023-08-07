/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    Node.cpp
*  @brif:    uniform interface
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*  @details:
*
******************************************************************************/
#include <GLSLProgram.h>
#include "TextRender.h"
#include "Node.h"

Node::Node()
{
    averageFps = 0.0;
}

Node::NodeType Node::getNodeType()
{
    return NodeType_Generic;
}

std::string Node::getNodeTypeStringByType(Node::NodeType type)
{
    switch (type) {
        case NodeType_Generic:
            return "Generic";
        case NodeType_FillRate:
            return "FillRate";
        default:
            return "Undefined";
    }
}

void Node::setWeightValue(int weight)
{
    weightValue = std::min(Node::MAX_WEIGHT, std::max(weight, Node::MIN_WEIGHT));
}

int Node::getWeightValue()
{
    return weightValue;
}

int Node::getWidth() const
{
    return width;
}

int Node::getHeight() const
{
    return height;
}

float Node::getWindowRatio() const
{
    return width / (float)height;
}

Node::RunningState Node::run()
{
    RunningState result = RunningState_Failed;
    frameNum = 0;
    perfWindow = PerfWindow::get();
    perfWindow->setWindowTitle(std::string("GPU_Perf_GLES_2_0 [ " + nodeName + " ]"));
    textRender = new TextRender();
    auto init = textRender->init("../media/fonts/NotoSansMono.ttf", 14);

    if (!init) {
        std::cout << "init text render failed, bad text shader?\n";
        return result;
    }

    textRender->onResize(width, height);

    startTime = PerfWindow::perfGetTime();
    lastTime = startTime;

    double priv_time = startTime;
    perfWindow->setRenderNode(this);

    if (!startup())
        return result;

    result = RunningState_Success;

    double difDelay = 0.0001;
    double startTimeVar = startTime;
    bool firstFps = true;

    char fpsStr[30] = {0};

    do {
        double current = PerfWindow::perfGetTime();
        double dif = current - priv_time;
        render(current, dif);

        if (firstFps) {
            firstFps = false;
            difDelay = dif;
        }

        if (current - startTimeVar > 0.9) {
            startTimeVar = current;
            difDelay = dif;
        }

        sprintf(fpsStr, "fps:%.2f", 1.0 / difDelay);
        textRender->render(fpsStr);

        perfWindow->swapBuffer();

        frameNum++;
        priv_time = current;
        lastTime = priv_time - startTime;

        uint32_t error = glGetError();
        /*if (error != GL_NO_ERROR) {
            result = RunningState_Failed;
        }*/
        if (!perfWindow->processInput()) {
            result = RunningState_Abort;
            break;
        }
    } while (lastTime < totalTime);

    delete textRender;
    textRender = nullptr;

    shutdown();
    endTime = PerfWindow::perfGetTime();
    averageFps = static_cast<float>(frameNum / ((endTime - startTime)));
    return result;
}

float Node::getAverageFps()
{
    return averageFps;
}

std::string Node::getNodeName() const
{
    return nodeName;
}

bool Node::startup()
{
    return true;
}

void Node::render(double currentTime, double difTime)
{
    (void)currentTime;
    (void)difTime;
}

void Node::shutdown()
{
}

void Node::onResized(int width, int height)
{
    if (height == 0)
        height = 1;

    this->width = width;
    this->height = height;
    glViewport(0, 0, width, height);

    if (textRender) {
        textRender->onResize(width, height);
    }
}

int Node::isExtensionSupported(const char *extname)
{
    (void)extname;
    /*GLint numExtensions;
    GLint i;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (i = 0; i < numExtensions; i++) {
        const GLubyte *e = glGetStringi(GL_EXTENSIONS, i);
        if (!strcmp((const char *)e, extname)) {
            return 1;
        }
    }*/

    return 0;
}

void Node::setAttribLocation(GLuint *attribLocations, const char **attribNames, \
                             GLSLProgram &shaderProgram, int count)
{
    shaderProgram.use();
    for (int i = 0; i < count; i++) {
        attribLocations[i] = static_cast<GLuint>(shaderProgram.getAttriLocation(attribNames[i]));
    }
}
