#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

using namespace std;

const char *dev = "/dev/dri/card0";

int main() {
    int err = 0;
    int fd = open(dev, O_RDONLY);
    if (fd < 0) {
        int err = errno;
        fprintf(stderr, "open %s failed: %s\n", dev, strerror(err));
        exit(1);
    }

    drmVersionPtr v = drmGetVersion(fd);
    if (v) {
        cout << "driver name: " << v->name << endl;
        cout << "driver description: " << v->desc << endl;
        drmFreeVersion(v);
    }

    drmModePlaneRes *plane_res = drmModeGetPlaneResources(fd);
    if (!plane_res) {
        err = errno;
        fprintf(stderr, "drmModeGetPlaneResources failed: %s\n", strerror(err));
        exit(1);
    }

    printf("planes count: %d\n", plane_res->count_planes);

    for (int i = 0; i < plane_res->count_planes; i++) {
        drmModePlane *plane = drmModeGetPlane(fd, plane_res->planes[i]);
        if (!plane) {
            err = errno;
            fprintf(stderr, "drmModeGetPlane failed: %s\n", strerror(err));
            continue;
        }

        if (plane->fb_id == 0) {
            drmModeFreePlane(plane);
            continue;
        }

        drmModeFreePlane(plane);
    }

    drmModeFreePlaneResources(plane_res);

    drmModeRes *res = drmModeGetResources(fd);
    if (!res) {
        err = errno;
        fprintf(stderr, "drmModeGetResources failed: %s\n", strerror(err));
        exit(1);
    }

    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector *connector = drmModeGetConnector(fd, res->connectors[i]);
        if (!connector) {
            err = errno;
            fprintf(stderr, "drmModeGetConnector failed: %s\n", strerror(err));
            continue;
        }

        if (connector->connection == DRM_MODE_CONNECTED) {
            printf("connector %d connected\n", connector->connector_id);

            drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoder_id);
            if (!encoder) {
                err = errno;
                fprintf(stderr, "drmModeGetEncoder failed: %s\n", strerror(err));
                continue;
            }

            drmModeCrtc *crtc = drmModeGetCrtc(fd, encoder->crtc_id);
            if (!crtc) {
                err = errno;
                fprintf(stderr, "drmModeGetCrtc failed: %s\n", strerror(err));
                continue;
            }

            printf("crtc: %d, encoder: %d, connector: %d\n", crtc->crtc_id, encoder->encoder_id, connector->connector_id);

            drmModeFreeCrtc(crtc);
            drmModeFreeEncoder(encoder);
        } else {
            printf("connector %d disconnected\n", connector->connector_id);
        }

        drmModeFreeConnector(connector);
    }

    drmModeFreeResources(res);

    close(fd);
    return 0;
}
