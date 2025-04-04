#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <iostream>

struct {
    XRRScreenResources *res;
    XRROutputInfo **outputs;
    XRRCrtcInfo **crtcs;
    int event_base;
    int min_width;
    int max_width;
    int min_height;
    int max_height;
    int num_monitors;
} randr;

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        std::cerr << "XOpenDisplay failed" << std::endl;
        return 1;
    }

    Window root = RootWindow(display, DefaultScreen(display));
    randr.res = XRRGetScreenResourcesCurrent(display, root);
    randr.outputs = (XRROutputInfo **) malloc(sizeof(XRROutputInfo *) * randr.res->noutput);
    randr.crtcs = (XRRCrtcInfo **) malloc(sizeof(XRRCrtcInfo) * randr.res->ncrtc);
    for (int i = 0; i < randr.res->noutput; ++i) {
        randr.outputs[i] = XRRGetOutputInfo(display, randr.res, randr.res->outputs[i]);
        if (randr.outputs[i]->connection == RR_Connected) {
            randr.num_monitors++;
        }
    }
    for (int i = 0; i < randr.res->ncrtc; ++i) {
        randr.crtcs[i] = XRRGetCrtcInfo(display, randr.res, randr.res->crtcs[i]);
    }
    if (XRRGetScreenSizeRange(display, root,
            &randr.min_width,
            &randr.min_height,
            &randr.max_width,
            &randr.max_height) != 1) {
        std::cerr << "XRRGetScreenSizeRange failed" << std::endl;
    }

    std::cout << "----------------------------------------" << std::endl;
    for (int i = 0; i < randr.res->noutput; ++i) {
        std::cout << "Ouput #" << i << ", ID: " << randr.res->outputs[i] << std::endl;
        std::cout << "  name: " << randr.outputs[i]->name;
        std::cout << (randr.outputs[i]->connection == RR_Connected ? "  connected" : "  disconnected");
        std::cout << "  " << randr.outputs[i]->mm_width << "x" << randr.outputs[i]->mm_height << std::endl;
        std::cout << "  CRTC: " << randr.outputs[i]->crtc << std::endl;
        std::cout << "  CRTCs: [ ";
        for (int j = 0; j < randr.outputs[i]->ncrtc; ++j) {
            std::cout << randr.outputs[i]->crtcs[j] << " ";
        }
        std::cout << "]" << std::endl;
        std::cout << std::endl;
    }

    std::cout << "----------------------------------------" << std::endl;
    for (int i = 0; i < randr.res->ncrtc; ++i) {
        std::cout << "CRTC #" << i << ", ID: " << randr.res->crtcs[i] << std::endl;
        std::cout << "  " << randr.crtcs[i]->width << "x" << randr.crtcs[i]->height << "+" << randr.crtcs[i]->x << "+" << randr.crtcs[i]->y;
        std::cout << "  mode: " << randr.crtcs[i]->mode << std::endl;
        std::cout << "  Outputs: [ ";
        for (int j = 0; j < randr.crtcs[i]->noutput; ++j) {
            std::cout << randr.crtcs[i]->outputs[j] << " ";
        }
        std::cout << "]" << std::endl;
        std::cout << std::endl;
    }

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "min-max: " << randr.min_width << "x" << randr.min_height;
    std::cout << "-" << randr.max_width << "x" << randr.max_height << std::endl;

    for (int i = 0; i < randr.res->noutput; ++i) {
        XRRFreeOutputInfo(randr.outputs[i]);
    }
    for (int i = 0; i < randr.res->ncrtc; ++i) {
        XRRFreeCrtcInfo(randr.crtcs[i]);
    }
    free(randr.outputs);
    free(randr.crtcs);
    XRRFreeScreenResources(randr.res);
    XCloseDisplay(display);

    return 0;
}

// g++ -o xrandr_info xrandr_info.cc -std=c++11 -lXrandr -lX11
