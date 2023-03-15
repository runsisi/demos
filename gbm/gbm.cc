#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gbm.h>
#include <xf86drmMode.h>

int main() {
    int err = 0;
    int fd = open("/dev/dri/card0", O_RDWR | FD_CLOEXEC);
    if (fd < 0) {
        int err = errno;
        fprintf(stderr, "open card0 failed: %s\n", strerror(err));
        exit(1);
    }

    struct gbm_device *gbm = gbm_create_device(fd);
    if (!gbm) {
        fprintf(stderr, "gbm_create_device failed\n");
        exit(1);
    }

    drmModePlaneRes *res = drmModeGetPlaneResources(fd);
    if (!res) {
        err = errno;
        fprintf(stderr, "drmModeGetPlaneResources failed: %s\n", strerror(err));
        exit(1);
    }

    for (int i = 0; i < res->count_planes; i++) {
        drmModePlane *plane = drmModeGetPlane(fd, res->planes[i]);
        if (!plane) {
            err = errno;
            fprintf(stderr, "drmModeGetPlane failed: %s\n", strerror(err));
            continue;
        }

        if (plane->fb_id == 0) {
            drmModeFreePlane(plane);
            continue;
        }
    }

    gbm_bo_import(gbm, GBM_BO_IMPORT_FD, NULL, 0);

    gbm_device_destroy(gbm);
    drmModeFreePlaneResources(res);
    close(fd);
}
