#include <X11/Xlib.h>

#include <stdio.h>

int main(int argc, char** argv) {
    Display* display;
    Window window;
    Screen* screen;
    int screenId;
    XEvent ev;

    // Open the display
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        printf("%s\n", "Could not open display");
        return 1;
    }
    screen = DefaultScreenOfDisplay(display);
    screenId = DefaultScreen(display);

    // Open the window
    window = XCreateSimpleWindow(display, RootWindowOfScreen(screen),
        0, 0, 320, 200, 1,
        BlackPixel(display, screenId),
        WhitePixel(display, screenId));

    // Show the window
    XClearWindow(display, window);
    XMapRaised(display, window);

    // Enter message loop
    while (true) {
        XNextEvent(display, &ev);
        // Do something
    }

    // Cleanup
    XDestroyWindow(display, window);
    XFree(screen);
    XCloseDisplay(display);
    return 1;
}
