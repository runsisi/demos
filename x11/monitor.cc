#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <stdio.h>

int main() {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        return 1;
    }

    int err = 0;

    int n;

    Window root = RootWindow (dpy, DefaultScreen(dpy));
    XRRMonitorInfo *monitors = XRRGetMonitors(dpy, root, true, &n);
    if (n == -1) {
        err = 1;
        goto out;
    }

    for (int i = 0; i < n; i++) {
        XRRMonitorInfo *m = &monitors[i];
        printf("%dx%d+%d+%d\n", m->width, m->height, m->x, m->y);
    }

    XRRFreeMonitors(monitors);

out:
    XCloseDisplay(dpy);
    return err;
}
