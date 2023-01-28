#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <EGL/egl.h>

#include <GL/gl.h>

#define WIDTH 4
#define HEIGHT 4

int main(int argc, char *argv[])
{
    int gles = argc > 1 ? !strcmp(argv[1], "-es") : 0;

    EGLDisplay egl_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!egl_dpy)
        return -1;

    EGLint major, minor;
    int ret = eglInitialize(egl_dpy, &major, &minor);
    if (!ret)
        return -1;

    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, gles ? EGL_OPENGL_ES_BIT : EGL_OPENGL_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };
    EGLint nb_configs;
    EGLConfig egl_cfg;
    ret = eglChooseConfig(egl_dpy, attribs, &egl_cfg, 1, &nb_configs);
    if (!ret)
        return -1;

    eglBindAPI(gles ? EGL_OPENGL_ES_API : EGL_OPENGL_API);

    EGLContext egl_ctx = eglCreateContext(egl_dpy, egl_cfg, EGL_NO_CONTEXT, NULL);
    if (!egl_ctx)
        return -1;

    const EGLint pbuffer_attribs[] = {
        EGL_WIDTH, WIDTH,
        EGL_HEIGHT, HEIGHT,
        EGL_NONE,
    };
    EGLSurface egl_surface = eglCreatePbufferSurface(egl_dpy, egl_cfg, pbuffer_attribs);
    if (!egl_surface)
        return -1;

    ret = eglMakeCurrent(egl_dpy, egl_surface, egl_surface, egl_ctx);
    if (!ret)
        return -1;

    uint8_t *data = (uint8_t *)calloc(1, 4 * WIDTH * HEIGHT);
    if (!data)
        return -1;

    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, data);

    for(int y = 0; y < HEIGHT; y++) {
        for(int x = 0; x < WIDTH; x++) {
            int *color = (int *)data + x*y;
            printf("0x%8x ", *color);
        }
        printf("\n");
    }
    free(data);

    eglTerminate(egl_dpy);
    return 0;
}

// Reading back an EGL Pbuffer using the OpenGL API returns garbled output
// https://gitlab.freedesktop.org/mesa/mesa/-/issues/164
