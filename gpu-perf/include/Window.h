#ifndef WINDOW_H
#define WINDOW_H
#include <cmath>
#include <cstdio>
#include <cstring>
#ifdef WINDOW_USE_GLFW
    #include <GLFW/glfw3.h>
#endif
#include <glad/glad.h>
#ifdef WINDOW_USE_XEGL
    #include <perfeglcontext.h>
    #include <perfxwindow.h>
    #include <perftimer.h>
#endif
#include "Log.h"

class Node;
class PerfWindow
{
public:
    static PerfWindow *get();
    ~PerfWindow();
public:
    bool create();
    void setWindowSize(int width, int height);
    void setWindowTitle(const char *title);
    void setOpenGLVersion(unsigned int major, unsigned int minor);
    void setRenderNode(Node *node);
    void swapBuffer();
    bool processInput();
    void setWindowTitle(const std::string &title);
    bool isOpenGLSupported();

    int getGLESVersion();
    void setFullScreen(bool);
#ifdef WINDOW_USE_XEGL
    EGLglproc ctxGetProcAddress(const char *funcName);
#endif

public:
#ifdef WINDOW_USE_GLFW
    static void onWindowResized(GLFWwindow *window, int width, int height);
#endif
#ifdef WINDOW_USE_XEGL
    static EGLglproc funcGetProcAddress(const char *funcName);
#endif
    static double perfGetTime();
private:
    PerfWindow();
    bool destory();
private:
    static PerfWindow *perfWin;
private:
    //static int majorVersion;
    //static int minorVersion;
    int glesVersion;
    bool fullScreen;
    int windowWidth;
    int windowHeight;
    int width;
    int height;
#ifdef WINDOW_USE_GLFW
    GLFWwindow *glfwWindow = nullptr;
#endif
#ifdef WINDOW_USE_XEGL
    PerfEGLContext perfEGLCtx;
#endif
    Node *renderNode = nullptr;

};

#endif /*__WINDOW_H__*/
