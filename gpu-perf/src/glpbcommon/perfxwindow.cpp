#include <iostream>
#include <string.h>
#include <perfxwindow.h>

PerfXWindow::PerfXWindow()
{
    xDisplay = nullptr;
    xWin = 0;
}

PerfXWindow::~PerfXWindow()
{
    clear();
}

bool PerfXWindow::createXWindow(int width, int height)
{
    /*XInitThreads();
    XrmInitialize();
    XSetErrorHandler(errorHandler);*/
    xDisplay = XOpenDisplay(NULL);
    if (xDisplay == NULL) {
        std::cerr << "cannot connect to X server\n" << std::endl;
        return false;
    }

    Window root = DefaultRootWindow(xDisplay);

    XSetWindowAttributes  swa;
    swa.event_mask  =  ExposureMask | ButtonPressMask | KeyPressMask;

    xWin = XCreateWindow(xDisplay, root,
                         0, 0, width, height, 0,
                         CopyFromParent, InputOutput,
                         CopyFromParent, CWEventMask,
                         &swa);
    XMapWindow(xDisplay, xWin);

    XSetWindowAttributes xattr;

    xattr.override_redirect = False;
    XChangeWindowAttributes(xDisplay, xWin, CWOverrideRedirect, &xattr);

    int one = 0;
    XChangeProperty(
        xDisplay, xWin,
        XInternAtom ( xDisplay, "_HILDON_NON_COMPOSITED_WINDOW", False ),
        XA_INTEGER,  32,  PropModeReplace,
        (unsigned char *) &one,  1);

    XWMHints hints;
    hints.input = True;
    hints.flags = InputHint;
    XSetWMHints(xDisplay, xWin, &hints);

    XSizeHints sizehints;
    sizehints.x = 0;
    sizehints.y = 0;
    sizehints.width  = width;
    sizehints.height = height;
    sizehints.flags = USSize | USPosition;
    XSetNormalHints(xDisplay, xWin, &sizehints);
    XSetStandardProperties(xDisplay, xWin, "", "", None, (char **)NULL, 0, &sizehints);

    return true;
}

Window PerfXWindow::getXWindow()
{
    return xWin;
}

Display *PerfXWindow::getXDisplay()
{
    return xDisplay;
}

void PerfXWindow::flush()
{
    XFlush(xDisplay);
}

void PerfXWindow::setTitle(const char *title)
{
#if defined(X_HAVE_UTF8_STRING)
    Xutf8SetWMProperties(xDisplay, xWin, title, title, NULL, 0, NULL, NULL, NULL);
#else
    // This may be a slightly better fallback than using XStoreName and
    // XSetIconName, which always store their arguments using STRING
    XmbSetWMProperties(xDisplay, xWin, title, title,  NULL, 0, NULL, NULL, NULL);
#endif
    Atom xNetWMName = XInternAtom(xDisplay, "_NET_WM_ICON_NAME", False);
    Atom xUtf8String = XInternAtom(xDisplay, "UTF8_STRING", False);
    XChangeProperty(xDisplay, xWin, xNetWMName, xUtf8String, 8, PropModeReplace,
                    (unsigned char *) title, strlen(title));
    Atom xNetWMIconName = XInternAtom(xDisplay, "_NET_WM_ICON_NAME", False);
    XChangeProperty(xDisplay, xWin, xNetWMIconName, xUtf8String, 8, PropModeReplace,
                    (unsigned char *) title, strlen(title));
    flush();
}

void PerfXWindow::clear()
{
    if (xDisplay) {
        XDestroyWindow(xDisplay, xWin);
        xWin = (Window) 0;
        XFlush(xDisplay);
        XCloseDisplay(xDisplay);
        xDisplay = 0;
    }
}

int PerfXWindow::errorHandler(Display *dpy, XErrorEvent *error)
{
    char buffer[512];
    XGetErrorText(dpy, error->error_code, buffer, sizeof buffer);
    std::cerr << "error: xlib: " << buffer;

    if (error->request_code < 128) {
        std::string request_code = std::to_string(error->request_code);
        XGetErrorDatabaseText(dpy, "XRequest", request_code.c_str(), "", buffer, sizeof buffer);
        std::cerr << " in " << buffer;
    }

    std::cerr << "\n";
    return 0;
}
