#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <xf86drm.h>

using namespace std;

const char *dev = "/dev/dri/card0";

int main() {
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

    return 0;
}
