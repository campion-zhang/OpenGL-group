/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    Window.h
*  @brif:    uniform interface
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <assert.h>
#include <sstream>
#include <regex>
#include "dlfcn.h"
#include "Window.h"
#include "Node.h"

//#define WINDOW_USE_XEGL 1
//#define WINDOW_USE_GLFW 1
#ifndef PERF_EGL_PATH
    #define PERF_EGL_PATH "libEGL.so"
#endif
#ifndef PERF_GLES_PATH
    #define PERF_GLES_PATH "libGLESv2.so"
#endif

PerfWindow *PerfWindow::perfWin = nullptr;

bool PerfWindow::isOpenGLSupported()
{
#ifdef WINDOW_USE_GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *glfwWin = glfwCreateWindow(200, 200, "opengl version", nullptr, nullptr);
    if (!glfwWin) {
        fprintf(stderr, "failed to open window\n");
        return false;
    }

    glfwMakeContextCurrent(glfwWin);
    glfwHideWindow(glfwWin);

    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "failed to initialize glad" << std::endl;
        return false;
    }

    const unsigned char *version = glGetString(GL_VERSION);
    if (!version || strlen(reinterpret_cast<const char *>(version)) < 3) {
        printf("System OpenGL Version Get Failed !!!!\n");
        return false;
    }

    const char *sVersion = reinterpret_cast<const char *>(version);
    std::cmatch m;
    std::regex versionRegex("([oO]pen[gG][lL][ ]*[eE][sS][ ]*([0-9]+)[\.]([0-9]+))");
    bool ret = std::regex_search(sVersion, m, versionRegex);
    if (ret && m.size() >= 4) {
        int major = stoi(m[2].str());
        int minor = stoi(m[3].str());
        glesVersion = major * 100 + minor * 10;
        if (major < 2) {
            std::cout << "version is not OpenGL ES 2.0" << std::endl;
            return false;
        }

    } else {
        std::cout << "version is not OpenGL ES 2.0" << std::endl;
        return false;
    }

    glfwDestroyWindow(glfwWin);
    glfwTerminate();
#endif

    return true;
}

PerfWindow::PerfWindow()
{
    width = 800;
    height = 600;
    glesVersion = 0;
    fullScreen = false;
    windowWidth = width;
    windowHeight = height;
    renderNode = nullptr;
}

PerfWindow::~PerfWindow()
{
    if (!destory()) {
        Log::error("Destory Window Error");
    }
}

#ifdef WINDOW_USE_GLFW
void PerfWindow::onWindowResized(GLFWwindow *window, int width, int height)
{
    (void)window;
    get()->setWindowSize(width, height);
}
#endif

PerfWindow *PerfWindow::get()
{
    if (perfWin == 0)
        perfWin = new PerfWindow();
    return perfWin;
}

void PerfWindow::swapBuffer()
{
#ifdef WINDOW_USE_GLFW
    glfwSwapBuffers(glfwWindow);
    glfwPollEvents();
#endif

#ifdef WINDOW_USE_XEGL
    glFinish();
    perfEGLCtx.swapBuffer();
#endif
}

bool PerfWindow::processInput()
{
#ifdef WINDOW_USE_GLFW
    if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
            glfwWindowShouldClose(glfwWindow)) {
        glfwSetWindowShouldClose(glfwWindow, true);
        return false;
    }
#endif
    return true;
}

void PerfWindow::setRenderNode(Node *node)
{
    if (node != renderNode && node) {
        renderNode = node;
        setWindowSize(width, height);
    }
}

void PerfWindow::setWindowSize(int w, int h)
{
    width = w;
    height = h;
    if (renderNode) {
        renderNode->onResized(w, h);
    }
}

void PerfWindow::setWindowTitle(const char *title)
{
#ifdef WINDOW_USE_GLFW
    glfwSetWindowTitle(glfwWindow, title);
#endif

}

bool PerfWindow::create()
{
#ifdef WINDOW_USE_GLFW
    if (glfwWindow != nullptr)
        return true;

    if (!glfwInit()) {
        fprintf(stderr, "failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 16);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    int desktopWidth = width;
    int desktopHeight = height;

    int monitorCount;
    GLFWmonitor *monitor = nullptr;
    GLFWmonitor **pMonitor =  glfwGetMonitors(&monitorCount);

    for (int i = 0; i < monitorCount;) {
        const GLFWvidmode *mode = glfwGetVideoMode(pMonitor[i]);
        desktopWidth = mode->width;
        desktopHeight = mode->height;
        monitor = pMonitor[i];
        i++;
        break;
    }

    if (fullScreen) {
        width = desktopWidth;
        height = desktopHeight;
    }

    glfwWindow = glfwCreateWindow(width, height, "GPU_Perf_GLES_2_0", fullScreen ? monitor : nullptr,
                                  nullptr);
    if (!glfwWindow) {
        fprintf(stderr, "failed to open window\n");
        return false;
    }

    if (!fullScreen) {
        int x = (desktopWidth - width) >> 1;
        int y = (desktopHeight - height) >> 1;
        glfwSetWindowPos(glfwWindow, x, y);
    }

    glfwSetWindowSizeCallback(glfwWindow, &PerfWindow::onWindowResized);
    glfwMakeContextCurrent(glfwWindow);

    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "create failed to initialize glad" << std::endl;
        return false;
    }

    const unsigned char *version = glGetString(GL_VERSION);
    if (!version || strlen(reinterpret_cast<const char *>(version)) < 3) {
        printf("System OpenGL Version Get Failed !!!!\n");
        return false;
    }

    const char *sVersion = reinterpret_cast<const char *>(version);
    std::cmatch m;
    std::regex versionRegex("([oO]pen[gG][lL][ ]*[eE][sS][ ]*([0-9]+)[\\.]([0-9]+))");
    bool ret = std::regex_search(sVersion, m, versionRegex);
    if (ret && m.size() >= 4) {
        int major = stoi(m[2].str());
        int minor = stoi(m[3].str());
        glesVersion = major * 100 + minor * 10;
        if (major < 2) {
            std::cout << "version is not OpenGL ES 2.0" << std::endl;
            return false;
        }

    } else {
        std::cout << "version is not OpenGL ES 2.0" << std::endl;
        return false;
    }
#endif

#ifdef WINDOW_USE_XEGL

    PerfTimer::initTimer();

    if (!perfEGLCtx.loadHandle(PERF_EGL_PATH, PERF_GLES_PATH)) {
        std::cout << "egl context load failed" << std::endl;
        return false;
    }

    if (!perfEGLCtx.createContext(width, height)) {
        std::cout << "egl createContext failed" << std::endl;
        return false;
    }
    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(PerfWindow::funcGetProcAddress))) {
        std::cout << "failed to initialize glad" << std::endl;
        return false;
    }
    glesVersion = perfEGLCtx.getGLESVersion();

#endif

    std::stringstream ss;

    ss << "    OpenGL Information" << std::endl;
    ss << "    GL_VENDOR:     " << glGetString(GL_VENDOR) << std::endl;
    ss << "    GL_RENDERER:   " << glGetString(GL_RENDERER) << std::endl;
    ss << "    GL_VERSION:    " << glGetString(GL_VERSION) << std::endl;

    Log::info("%s", ss.str().c_str());
    Log::info("=======================================================================\n");

    return true;
}

bool PerfWindow::destory()
{
#ifdef WINDOW_USE_GLFW
    if (glfwWindow != nullptr) {
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }
#endif

#ifdef WINDOW_USE_XEGL
    perfEGLCtx.clear();
#endif

    return true;
}

void PerfWindow::setWindowTitle(const std::string &title)
{
#ifdef WINDOW_USE_GLFW
    glfwSetWindowTitle(glfwWindow, title.c_str());
#endif
}

int PerfWindow::getGLESVersion()
{
    return glesVersion;
}

void PerfWindow::setFullScreen(bool isFullScreen)
{
    fullScreen = isFullScreen;
}

#ifdef WINDOW_USE_XEGL
EGLglproc PerfWindow::ctxGetProcAddress(const char *funcName)
{
    return perfEGLCtx.ctxGetProcAddress(funcName);
}

EGLglproc PerfWindow::funcGetProcAddress(const char *funcName)
{
    return get()->ctxGetProcAddress(funcName);
}
#endif

double PerfWindow::perfGetTime()
{
#ifdef WINDOW_USE_GLFW
    return glfwGetTime();
#endif

#ifdef WINDOW_USE_XEGL
    return PerfTimer::perfGetTime();
#endif
    return 0;
}
