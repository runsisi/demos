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

    drmModePlaneRes *res = drmModeGetPlaneResources(fd);
    if (!res) {
        err = errno;
        fprintf(stderr, "drmModeGetPlaneResources failed: %s\n", strerror(err));
        exit(1);
    }

    printf("planes count: %d\n", res->count_planes);

    drmModeFreePlaneResources(res);
    close(fd);
    return 0;
}
