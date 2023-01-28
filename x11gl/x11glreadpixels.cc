#include <fmt/core.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>

#define WIDTH 128
#define HEIGHT 128

static GLuint pbo = 0;

#define checkGLError() \
do { \
    GLenum err; \
    /* mesa always clears the last error, so the loop is not needed */ \
    while ((err = glGetError()) != GL_NO_ERROR) { \
        fmt::print("GL error: 0x{:x} @line {}\n", err, __LINE__); \
    } \
} while(0)

static bool isExtensionSupported(const char *exts, const char *ext) {
    return strstr(exts, ext) != NULL;
}

int main() {
    // Open the display
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fmt::print("Could not open display\n");
        return 1;
    }

    // todo: using scope guard to close x display

    // Check GLX version
    GLint majorGLX, minorGLX = 0;
    glXQueryVersion(dpy, &majorGLX, &minorGLX);
    if (majorGLX <= 1 && minorGLX < 2) {
        fmt::print("GLX 1.2 or greater is required.\n");
        exit(1);
    }

    int screen = DefaultScreen(dpy);

    fmt::print("GLX client version: {}\n", glXGetClientString(dpy, GLX_VERSION));
    fmt::print("GLX client vendor: {}\n", glXGetClientString(dpy, GLX_VENDOR));
    fmt::print("GLX client extensions:\n\t{}\n", glXGetClientString(dpy, GLX_EXTENSIONS));

    fmt::print("GLX server version: {}\n", glXQueryServerString(dpy, screen, GLX_VERSION));
    fmt::print("GLX server vendor: {}\n", glXQueryServerString(dpy, screen, GLX_VENDOR));
    fmt::print("GLX server extensions:\n\t{}\n", glXQueryServerString(dpy, screen, GLX_EXTENSIONS));

    GLXFBConfig fbc;
    {
        GLint glxAttrs[] = {
            GLX_X_RENDERABLE, True,
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,
            GLX_DEPTH_SIZE, 24,
            GLX_STENCIL_SIZE, 8,
            GLX_DOUBLEBUFFER, True,
            None
        };

        int fbc_count;
        GLXFBConfig *fbcs = glXChooseFBConfig(dpy, screen, glxAttrs, &fbc_count);
        if (!fbcs) {
            fmt::print("Failed to retrieve framebuffer\n");
            exit(1);
        }

        fmt::print("Found {} matching framebuffers\n", fbc_count);

        // Pick the FB config/visual with the most samples per pixel
        fmt::print("Getting best XVisualInfo\n");

        int best_fbc_idx = -1, best_samples = -1;
        for (int i = 0; i < fbc_count; i++) {
            XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, fbcs[i]);
            if (vi) {
                int sample_buffers, samples;
                glXGetFBConfigAttrib(dpy, fbcs[i], GLX_SAMPLE_BUFFERS, &sample_buffers);
                glXGetFBConfigAttrib(dpy, fbcs[i], GLX_SAMPLES, &samples);

                if (best_fbc_idx < 0 || (sample_buffers && samples > best_samples)) {
                    best_fbc_idx = i;
                    best_samples = samples;
                }
            }
            XFree(vi);
        }

        fmt::print("Best visual info index: {}\n", best_fbc_idx);

        fbc = fbcs[best_fbc_idx];

        XFree(fbcs);
    }

    XVisualInfo *vi;
    {
        vi = glXGetVisualFromFBConfig(dpy, fbc);
        if (!vi) {
            fmt::print("Could not create correct visual window.\n");
            exit(1);
        }
    }

    Window win;
    XSetWindowAttributes winAttrs;
    {
        winAttrs.border_pixel = BlackPixel(dpy, screen);
        winAttrs.background_pixel = WhitePixel(dpy, screen);
        winAttrs.override_redirect = True;
        winAttrs.colormap = XCreateColormap(dpy, RootWindow(dpy, screen), vi->visual, AllocNone);
        winAttrs.event_mask = ExposureMask;
        win = XCreateWindow(dpy, RootWindow(dpy, screen), 0, 0,
            WIDTH, HEIGHT, 0, vi->depth,
            InputOutput, vi->visual,
            CWBackPixel | CWColormap | CWBorderPixel | CWEventMask /*| CWOverrideRedirect*/,
            &winAttrs);
    }

    GLXContext ctx;
    {
        const char *exts = glXQueryExtensionsString(dpy, screen);
        fmt::print("GLX extensions:\n\t{}\n\n", exts);
        if (!isExtensionSupported(exts, "GLX_ARB_create_context")) {
            ctx = glXCreateNewContext(dpy, fbc, GLX_RGBA_TYPE, NULL, True);
        } else {
            auto glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB(
                (const GLubyte *)"glXCreateContextAttribsARB");

            // we can either request GL < 3.2 or request compatibility profile explicitly,
            // see dri_convert_glx_attribs and driCreateContextAttribs in mesa
            int ctxAttrs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
                GLX_CONTEXT_MINOR_VERSION_ARB, 1,   // GL_PIXEL_PACK_BUFFER requires GL 2.1

                //
                // since
                // Forward-looking contexts are supported by silently converting the
                // requested API to API_OPENGL_CORE.
                // then
                // GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB conflicts with GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
                // also note
                // There are no forward-compatible contexts before OpenGL 3.0.
                //
                // GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,

                // request profile explicitly then we can request GL >= 3.2
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                None
            };
            ctx = glXCreateContextAttribsARB(dpy, fbc, NULL, True, ctxAttrs);
        }
    }

    XSync(dpy, False);

    // Verifying that context is a direct context
    if (!glXIsDirect(dpy, ctx)) {
        fmt::print("Indirect GLX rendering context obtained\n");
    } else {
        fmt::print("Direct GLX rendering context obtained\n");
    }

    // we should set current context before glGetString
    glXMakeCurrent(dpy, win, ctx);

    fmt::print("GL Vendor: {}\n", (char *)glGetString(GL_VENDOR));
    fmt::print("GL Renderer: {}\n", (char *)glGetString(GL_RENDERER));
    fmt::print("GL Version: {}\n", (char *)glGetString(GL_VERSION));
    fmt::print("GL Shading Language: {}\n", (char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    fmt::print("GL extensions:\n\t{}\n", exts);
    if (!isExtensionSupported(exts, "GL_ARB_pixel_buffer_object")) {
        fmt::print("GL_ARB_pixel_buffer_object extension not supported\n");
        exit(1);
    }

    // Show the window
    XClearWindow(dpy, win);
    XMapRaised(dpy, win);

    // requires GL 1.5
    // glvnd returns NULL only if dynamic stubs generated more than 4096 items
    auto glBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddressARB((const GLubyte *)"glBindBuffer");
    auto glGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddressARB((const GLubyte *)"glGenBuffers");
    auto glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glXGetProcAddressARB((const GLubyte *)"glDeleteBuffers");
    auto glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)glXGetProcAddressARB((const GLubyte *)"glGetBufferParameteriv");
    auto glBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddressARB((const GLubyte *)"glBufferData");
    auto glMapBuffer = (PFNGLMAPBUFFERPROC)glXGetProcAddressARB((const GLubyte *)"glMapBuffer");
    auto glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)glXGetProcAddressARB((const GLubyte *)"glUnmapBuffer");

    Atom WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &WM_DELETE_WINDOW, 1);

    bool running = true;
    while (running) {
        XWindowAttributes xwa;
        int pitch;

        XEvent ev;
        XNextEvent(dpy, &ev);
        switch (ev.type) {
        case Expose: {
            XGetWindowAttributes(dpy, win, &xwa);
            glViewport(0, 0, xwa.width, xwa.height);

            fmt::print("Window position, x: {}, y: {}, w: {}, h: {}\n", xwa.x, xwa.y, xwa.width, xwa.height);

            XShmSegmentInfo shm;
            XImage *xi = XShmCreateImage(dpy, xwa.visual, xwa.depth, ZPixmap, NULL, &shm, xwa.width, xwa.height);
            if (!xi) {
                fmt::print("XShmCreateImage failed\n");
                exit(1);
            }
            pitch = xi->bytes_per_line;
            XDestroyImage(xi);

            break;
        }
        case ClientMessage: {
            if (ev.xclient.data.l[0] == WM_DELETE_WINDOW) {
                running = false;
            }
            break;
        }
        default:
            break;
        }

        if (!running) {
            break;
        }

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_QUADS);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f(0.0f, -1.0f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(-1.0f, 0.0f);
        glEnd();

        // reading pixels w/ pbo

        if (!pbo) {
            glGenBuffers(1, &pbo);
            checkGLError();
        }
        if (!pbo) {
            fmt::print("glGenBuffers failed\n");
            exit(1);
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, pbo);
        checkGLError();

        int size = 0;
        int width = xwa.width;
        int height = xwa.height;

        glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
        checkGLError();
        if (size != pitch * height) {
            glBufferData(GL_PIXEL_PACK_BUFFER_EXT, pitch * height, NULL, GL_STREAM_READ);
            checkGLError();
        }
        glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
        checkGLError();
        if (size != pitch * height) {
            fmt::print("could not set pbo size\n");
            exit(1);
        }

        // we have requested render type of GLX_RGBA_BIT
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        checkGLError();

        char *pboBits = (char *)glMapBuffer(GL_PIXEL_PACK_BUFFER_EXT, GL_READ_ONLY);
        fmt::print("pboBits: 0x{:p}\n", pboBits);
        checkGLError();
        if (!pboBits) {
            fmt::print("could not map pixel buffer object\n");
            exit(1);
        }

        char *bits = (char *)malloc(pitch * height);
        memcpy(bits, pboBits, pitch * height);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT);
        checkGLError();
        glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
        checkGLError();

        fmt::print("-- pixels w/ pbo:\n");
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                unsigned *color = (unsigned *)bits + x * y;
                fmt::print("{:#8x} ", *color);
            }
            fmt::print("\n");
        }

        // reading pixels w/o pbo

        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bits);
        checkGLError();

        fmt::print("-- pixels w/o pbo:\n");
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                unsigned *color = (unsigned *)bits + x * y;
                fmt::print("{:#8x} ", *color);
            }
            fmt::print("\n");
        }
        free(bits);

        // Present frame
        glXSwapBuffers(dpy, win);
        checkGLError();
    }

    glDeleteBuffers(1, &pbo);

    // Cleanup GLX
    glXDestroyContext(dpy, ctx);

    // Cleanup X11
    XFree(vi);
    XFreeColormap(dpy, winAttrs.colormap);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}

// g++ -o x11glreadpixels x11glreadpixels.cc -lX11 -lXext -lGLX -lOpenGL -lfmt
