#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/extensions/XShm.h>

#include <fmt/core.h>
#include <iostream>

int main(int argc, char** argv) {
    Display* dpy;
    Window win;
    Screen* screen;
    int screenId;
    XEvent ev;

    // Open the display
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printf("%s\n", "Could not open display");
        return 1;
    }
    screen = DefaultScreenOfDisplay(dpy);
    screenId = DefaultScreen(dpy);

    // Open the window
    win = XCreateSimpleWindow(dpy, RootWindowOfScreen(screen), 0, 0, 320, 200, 1, BlackPixel(dpy, screenId), WhitePixel(dpy, screenId));

    XSelectInput(dpy, win, ExposureMask);

    // Name the window
    XStoreName(dpy, win, "Named Window");

    // Show the window
    XClearWindow(dpy, win);
    XMapRaised(dpy, win);

    // Resize window
    unsigned int mask = CWWidth | CWHeight;
    XWindowChanges values;
    values.width = 800;
    values.height = 600;
    XConfigureWindow(dpy, win, mask, &values);
    XSync(dpy, False);

    // How large is the window
    XWindowAttributes xwa;
    XGetWindowAttributes(dpy, win, &xwa);
    std::cout << "Window Width: " << xwa.width << ", Height: " << xwa.height << "\n";

    int err = XShmQueryExtension(dpy);
    if (!err) {
        fmt::print("XShm extension not supported\n");
        return 1;
    }

    XShmSegmentInfo shminfo;
    XImage *xi = XShmCreateImage(dpy, xwa.visual, xwa.depth, ZPixmap,
        NULL, &shminfo, xwa.width, xwa.height);
    if (!xi) {
        fmt::print("XShmCreateImage failed\n");
        return 1;
    }
    int pitch = xi->bytes_per_line;
    XDestroyImage(xi);

    fmt::print("pitch: {}\n", pitch);

    // Variables used in message loop
    bool running = true;

    // Enter message loop
    while (running) {
        XNextEvent(dpy, &ev);
        switch(ev.type) {
        case Expose:
            std::cout << "Expose event fired\n";
            XGetWindowAttributes(dpy, win, &xwa);
            std::cout << "\tWindow Width: " << xwa.width << ", Height: " << xwa.height << "\n";

            XShmSegmentInfo shminfo;
            XImage *xi = XShmCreateImage(dpy, xwa.visual, xwa.depth, ZPixmap,
                NULL, &shminfo, xwa.width, xwa.height);
            if (!xi) {
                fmt::print("XShmCreateImage failed\n");
                return 1;
            }
            int pitch = xi->bytes_per_line;
            XDestroyImage(xi);

            fmt::print("pitch: {}\n", pitch);
            break;
        }
    }

    // Cleanup
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 1;
}
