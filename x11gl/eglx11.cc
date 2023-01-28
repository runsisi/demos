#include <EGL/egl.h>
#include <GL/gl.h>

#include <X11/Xlib.h>

#include <iostream>
using namespace std;

// g++ -o eglx11 eglx11.cc -lEGL -lX11

int main() {
    auto xdpy = XOpenDisplay(NULL);
    if (!xdpy) {
        fprintf(stderr, "XOpenDisplay failed\n");
        exit(1);
    }

    auto dpy = eglGetDisplay(xdpy);
    if (dpy == EGL_NO_DISPLAY) {
        fprintf(stderr, "eglGetDisplay failed\n");
        exit(1);
    }

    auto eglInitialize = (PFNEGLINITIALIZEPROC)eglGetProcAddress("eglInitialize");
    EGLBoolean r = eglInitialize(dpy, NULL, NULL);
    if (!r) {
        EGLint err = eglGetError();
        fprintf(stderr, "eglInitialize failed: %d\n", err);
        exit(1);
    }

    const char *extensions = eglQueryString(dpy, EGL_EXTENSIONS);
    if (!extensions) {
        EGLint err = eglGetError();
        fprintf(stderr, "eglQueryString EGL_EXTENSIONS failed: %d\n", err);
        exit(1);
    }

    cout << extensions << endl;

    eglTerminate(dpy);
    XCloseDisplay(xdpy);
}
