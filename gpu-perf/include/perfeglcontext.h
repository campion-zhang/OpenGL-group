#pragma once
#include <glad/glad.h>
#include <EGL/egl.h>
//#include <perfxwindow.h>

typedef void (*EGLglproc)(void);
typedef void (*GLESproc)(void);
typedef EGLglproc (*PFN_eglGetProcAddress)(const char*);
typedef EGLDisplay (EGLAPIENTRY * PFN_eglGetDisplay)(EGLNativeDisplayType);
typedef EGLSurface (EGLAPIENTRY * PFN_eglCreateWindowSurface)(EGLDisplay,EGLConfig,EGLNativeWindowType,const EGLint*);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglBindAPI)(EGLenum);
typedef EGLContext (EGLAPIENTRY * PFN_eglCreateContext)(EGLDisplay,EGLConfig,EGLContext,const EGLint*);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglMakeCurrent)(EGLDisplay,EGLSurface,EGLSurface,EGLContext);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglChooseConfig)(EGLDisplay, EGLint const*, EGLConfig*, EGLint, EGLint*);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglInitialize)(EGLDisplay,EGLint*,EGLint*);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglQueryContext)(EGLDisplay, EGLContext, EGLint, EGLint*);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglSwapBuffers)(EGLDisplay,EGLSurface);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglDestroySurface)(EGLDisplay,EGLSurface);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglDestroyContext)(EGLDisplay,EGLContext);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglTerminate)(EGLDisplay);
typedef EGLBoolean (EGLAPIENTRY * PFN_eglSwapInterval)(EGLDisplay,EGLint);
typedef EGLSurface (EGLAPIENTRY * PFN_eglCreatePbufferSurface)(EGLDisplay, EGLConfig, const EGLint *);

struct PerfEGLLib
{
    PFN_eglGetProcAddress      getProcAddress;
    PFN_eglGetDisplay          getDisplay;
    PFN_eglCreateWindowSurface createWindowSurface;
    PFN_eglBindAPI             bindAPI;
    PFN_eglCreateContext       createContext;
    PFN_eglMakeCurrent         makeCurrent;
    PFN_eglChooseConfig        chooseConfig;
    PFN_eglInitialize          initialize;
    PFN_eglQueryContext        queryContext;
    PFN_eglSwapBuffers         swapBuffers;
    PFN_eglDestroySurface      destroySurface;
    PFN_eglDestroyContext      destroyContext;
    PFN_eglTerminate           terminate;
    PFN_eglSwapInterval        swapInterval;
    PFN_eglCreatePbufferSurface createPbufferSurface;
};

class PerfEGLContext
{
public:
    PerfEGLContext();
    ~PerfEGLContext();
public:
    bool loadHandle(const char*, const char*);
    GLESproc ctxGetProcAddress(const char*);
    bool createContext(int, int);
    void swapBuffer();
    void clear();
    int getGLESVersion();
private:
    void *eglHandle;
    void *glesHandle;
    EGLDisplay perfEGLDisplay;
    EGLSurface perfEGLSurface;
    EGLContext perfEGLCtx;
    GLint perfEGLMajor;
    GLint perfEGLMinor;
    PerfEGLLib perfEGLLib;
    GLint perfGLESMajor;
    GLint perfGLESMinor;
};
