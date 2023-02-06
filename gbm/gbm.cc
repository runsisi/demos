#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gbm.h>

int main() {
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

    gbm_device_destroy(gbm);
    close(fd);
}
