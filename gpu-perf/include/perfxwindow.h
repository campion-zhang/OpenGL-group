#pragma once
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

class PerfXWindow
{
public:
    PerfXWindow();
    ~PerfXWindow();
public:
    bool initDisplay();
    bool createXWindow(int, int);
    Window getXWindow();
    Display *getXDisplay();
    void flush();
    void setTitle(const char *);
    void clear();
public:
    static int errorHandler(Display *dpy, XErrorEvent *error);
private:
    Display *xDisplay = nullptr;
    Window xWin = 0;
    int emptyEventPipe[2] = {0};
    int monotonic = 0;
    int frequency = 0;
};
