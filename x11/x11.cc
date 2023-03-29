#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>

#include <stdio.h>

int main() {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        return 1;
    }

    int xdamageEv;
    int r;
    if (XDamageQueryExtension(dpy, &xdamageEv, &r)) {
        printf("DAMAGE extension available!\n");
    }

    XCloseDisplay(dpy);
    return 0;
}
