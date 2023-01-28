#include <fcntl.h>
#include <sys/stat.h>

#include <xf86drm.h>
#include <xcb/xcb.h>
#include <xcb/dri3.h>

#include <iostream>
#include <string>
using namespace std;

// g++ -o xcb xcb.cc -I/usr/include/libdrm -ldrm -lxcb-dri3 -lxcb

int main(int argc, char *argv[]) {
//    int fd = open("/dev/dri/card0", O_RDWR);
//    if (fd < 0) {
//        cout << "failed to open dri device\n";
//        return 1;
//    }

    if (argc != 2) {
        fprintf(stderr, "need display name\n");
        exit(1);
    }

    xcb_connection_t *conn = xcb_connect(argv[1], NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Failed to connect to the X server\n");
        exit(1);
    }

    xcb_screen_t *scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    xcb_dri3_open_cookie_t cookie = xcb_dri3_open(conn,
        scr->root,
        0);

    xcb_dri3_open_reply_t *reply =xcb_dri3_open_reply(conn, cookie, NULL);
    if (!reply) {
        fprintf(stderr, "xcb_dri3_open_reply failed\n");
        exit(1);
    }

    if (reply->nfd != 1) {
        free(reply);
        fprintf(stderr, "xcb_dri3_open_reply invalid reply\n");
        exit(1);
    }

    int fd = xcb_dri3_open_reply_fds(conn, reply)[0];
    cout << "dri3 fd: " << fd << endl;
    free(reply);
    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);

    struct stat sbuf{};
    if (fstat(fd, &sbuf)) {
        cout << "fstat failed: " << errno << endl;
        return 1;
    }
    if (!S_ISCHR(sbuf.st_mode)) {
        cout << "not a char dev\n";
        return 1;
    }

    cout << "x11 rdev: " << hex << sbuf.st_rdev << endl;

    int card0_fd = open("/dev/dri/card0", O_RDWR);
    if (card0_fd < 0) {
        cout << "failed to open card0 dri device\n";
        return 1;
    }
    struct stat card0_sbuf{};
    if (fstat(card0_fd, &card0_sbuf)) {
        cout << "card0 fstat failed: " << errno << endl;
        return 1;
    }
    if (!S_ISCHR(card0_sbuf.st_mode)) {
        cout << "card0 not a char dev\n";
        return 1;
    }

    int render_fd = open("/dev/dri/renderD128", O_RDWR);
    if (render_fd < 0) {
        cout << "failed to open render dri device\n";
        return 1;
    }
    struct stat render_sbuf{};
    if (fstat(render_fd, &render_sbuf)) {
        cout << "card0 fstat failed: " << errno << endl;
        return 1;
    }
    if (!S_ISCHR(render_sbuf.st_mode)) {
        cout << "render not a char dev\n";
        return 1;
    }

    cout << "card0 rdev: " << hex << card0_sbuf.st_rdev << endl;
    cout << "render rdev: " << hex << render_sbuf.st_rdev << endl;

    drmDevicePtr devinfo;

    /* Get PCI info. */
    int r = drmGetDevice2(fd, 0, &devinfo);
    if (r) {
        fprintf(stderr, "amdgpu: drmGetDevice2 failed: %d.\n", r);
        return 1;
    }

    cout << devinfo->bustype << endl;

    return 0;
}
