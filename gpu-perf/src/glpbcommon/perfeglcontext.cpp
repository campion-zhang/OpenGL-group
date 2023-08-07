#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <Log.h>
#include <perfeglcontext.h>

PerfEGLContext::PerfEGLContext()
{
    eglHandle = nullptr;
    glesHandle = nullptr;
    perfEGLDisplay = nullptr;
    perfEGLSurface = nullptr;
    perfEGLCtx = nullptr;
    perfEGLMajor = 0;
    perfEGLMinor = 0;
    perfGLESMajor = 0;
    perfGLESMinor = 0;
}

PerfEGLContext::~PerfEGLContext()
{

}

bool PerfEGLContext::loadHandle(const char *eglLibName, const char *glesLibName)
{

    eglHandle = dlopen(eglLibName, RTLD_LAZY | RTLD_LOCAL);
    if (!eglHandle) {
        std::cout << "dlopen "<< eglLibName << " failed" <<std::endl;
        return false;
    }
    Log::info("    eglHandle is %s\n", eglLibName);
    perfEGLLib.getProcAddress = (PFN_eglGetProcAddress)dlsym(eglHandle, "eglGetProcAddress");
    perfEGLLib.getDisplay = (PFN_eglGetDisplay)dlsym(eglHandle, "eglGetDisplay");
    perfEGLLib.createWindowSurface = (PFN_eglCreateWindowSurface)dlsym(eglHandle, "eglCreateWindowSurface");
    perfEGLLib.bindAPI = (PFN_eglBindAPI)dlsym(eglHandle, "eglBindAPI");
    perfEGLLib.createContext = (PFN_eglCreateContext)dlsym(eglHandle, "eglCreateContext");
    perfEGLLib.makeCurrent = (PFN_eglMakeCurrent)dlsym(eglHandle, "eglMakeCurrent");
    perfEGLLib.chooseConfig = (PFN_eglChooseConfig)dlsym(eglHandle, "eglChooseConfig");
    perfEGLLib.initialize = (PFN_eglInitialize)dlsym(eglHandle, "eglInitialize");
    perfEGLLib.queryContext = (PFN_eglQueryContext)dlsym(eglHandle, "eglQueryContext");
    perfEGLLib.swapBuffers = (PFN_eglSwapBuffers)dlsym(eglHandle, "eglSwapBuffers");
    perfEGLLib.destroySurface = (PFN_eglDestroySurface)dlsym(eglHandle, "eglDestroySurface");
    perfEGLLib.destroyContext = (PFN_eglDestroyContext)dlsym(eglHandle, "eglDestroyContext");
    perfEGLLib.terminate = (PFN_eglTerminate)dlsym(eglHandle, "eglTerminate");
    perfEGLLib.swapInterval = (PFN_eglSwapInterval)dlsym(eglHandle, "eglSwapInterval");
    perfEGLLib.createPbufferSurface = (PFN_eglCreatePbufferSurface)dlsym(eglHandle, "eglCreatePbufferSurface");
    if (!perfEGLLib.getProcAddress ||
        !perfEGLLib.getDisplay ||
        !perfEGLLib.createWindowSurface ||
        !perfEGLLib.bindAPI ||
        !perfEGLLib.createContext ||
        !perfEGLLib.makeCurrent ||
        !perfEGLLib.chooseConfig ||
        !perfEGLLib.initialize ||
        !perfEGLLib.queryContext ||
        !perfEGLLib.swapBuffers ||
        !perfEGLLib.destroySurface ||
        !perfEGLLib.destroyContext ||
        !perfEGLLib.terminate ||
        !perfEGLLib.swapInterval ||
        !perfEGLLib.createPbufferSurface) {
        std::cout << "dlsym eglGetProcAddress failed" <<std::endl;
        return false;
    }

    glesHandle = dlopen(glesLibName, RTLD_LAZY | RTLD_LOCAL);
    if (!glesHandle) {
        std::cout << "dlopen "<< glesLibName << " failed" <<std::endl;
        return false;
    }
    Log::info("    glesHandle is %s\n", glesLibName);
    Log::info("\n");
    return true;
}

GLESproc PerfEGLContext::ctxGetProcAddress(const char *funcName)
{
    if (!funcName) {
        std::cout << "funcName is null" << std::endl;
        return nullptr;
    }
    if (glesHandle) {
        GLESproc proc = (GLESproc)dlsym(glesHandle, funcName);
        if (proc) {
            //std::cout << "return GLESproc proc" << std::endl;
            return proc;
        }
    }

    if (!perfEGLLib.getProcAddress) {
        std::cout << "getProcAddress failed" << std::endl;
        return nullptr;
    }
    std::cout << "return getProcAddress" << std::endl;
    return perfEGLLib.getProcAddress(funcName);
}

bool PerfEGLContext::createContext(int width, int height)
{
    EGLContext ctx = nullptr;
    EGLConfig configs = nullptr;
    EGLint num_configs = 0;
    EGLint val = 0;
    static const EGLint attribs[] = {
            EGL_RED_SIZE, 1,
            EGL_GREEN_SIZE, 1,
            EGL_BLUE_SIZE, 1,
            EGL_ALPHA_SIZE, 1,
            EGL_DEPTH_SIZE, 1,
            EGL_STENCIL_SIZE, 0,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
    };
    static const EGLint ctx_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };

    /*EGL_DEFAULT_DISPLAY*/
    /*egl_display = eglGetDisplay((EGLNativeDisplayType) EGL_DEFAULT_DISPLAY);*/
    perfEGLDisplay = perfEGLLib.getDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
    if (!perfEGLDisplay) {
        std::cout << "Error: eglGetDisplay() failed\n" << std::endl;
        return false;
    }

    if (!perfEGLLib.initialize(perfEGLDisplay, &perfEGLMajor, &perfEGLMinor)) {
        std::cout << "Error: eglInitialize() failed\n" << std::endl;
        return false;
    }

    if (!perfEGLLib.chooseConfig(perfEGLDisplay, attribs, &configs, 1, &num_configs)) {
        std::cout << "Error: couldn't get an EGL visual config\n" << std::endl;
        return false;
    }
    if (!configs || num_configs <= 0) {
        std::cout << "eglChooseConfig failed" << std::endl;
        return false;
    }

    perfEGLLib.bindAPI(EGL_OPENGL_ES_API);
    ctx = perfEGLLib.createContext(perfEGLDisplay, configs, EGL_NO_CONTEXT, ctx_attribs );
    if (!ctx) {
        std::cout << "Error: eglCreateContext failed\n" << std::endl;
        return false;
    }

    perfEGLLib.queryContext(perfEGLDisplay, ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
    if (val < 2) {
        std::cout << "EGL_CONTEXT_CLIENT_VERSION < 2" << std::endl;
        return false;
    }
    perfGLESMajor = val;
    perfGLESMinor = 0;

    EGLint pAttr[] = {EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE};
    perfEGLSurface = perfEGLLib.createPbufferSurface(perfEGLDisplay, configs, pAttr);
    //perfEGLSurface = perfEGLLib.createWindowSurface(perfEGLDisplay, configs, -1, nullptr);
    if (!perfEGLSurface) {
        std::cout << "Error: eglcreateWindowSurface failed\n" << std::endl;
        return false;
    }

    if(!perfEGLLib.makeCurrent(perfEGLDisplay, perfEGLSurface, perfEGLSurface, ctx))
    {
        std::cout << "make current fail" << std::endl;
        return false;
    }
    return true;
}

void PerfEGLContext::swapBuffer()
{
    perfEGLLib.swapBuffers(perfEGLDisplay, perfEGLSurface);
}

void PerfEGLContext::clear()
{
    perfEGLLib.destroyContext(perfEGLDisplay, perfEGLCtx);
    perfEGLLib.destroySurface(perfEGLDisplay, perfEGLSurface);
    perfEGLLib.terminate(perfEGLDisplay);
    dlclose(eglHandle);
}

int PerfEGLContext::getGLESVersion()
{
    return (perfGLESMajor * 100) + (perfGLESMinor * 10);
}
