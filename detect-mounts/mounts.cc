//
// Created by runsisi on 9/8/23.
//

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>

// How could I detect when a directory is mounted with inotify?
// https://stackoverflow.com/questions/1113176/how-could-i-detect-when-a-directory-is-mounted-with-inotify

// the options string is apparently limited to the memory page size minus one

// Is there any way around the length limitation in mount options?
// https://unix.stackexchange.com/questions/563871/is-there-any-way-around-the-length-limitation-in-mount-options

// after opening the file for reading, a change in this file (i.e., a
// filesystem mount or unmount) causes select(2) to mark the
// file descriptor as having an exceptional condition, and
// poll(2) and epoll_wait(2) mark the file as having a
// priority event (POLLPRI)

// proc - process information, system information, and sysctl pseudo-filesystem
// https://man7.org/linux/man-pages/man5/proc.5.html

int main()
{
    int fd;
    struct pollfd ev;
    int r;
    char buf[4096];

    fd = open("/proc/mounts", O_RDONLY);
    while ((r = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, r);
    }

    for (;;) {
        ev.events = POLLPRI;
        ev.fd = fd;
        ev.revents = 0;
        r = poll(&ev, 1, -1);
        if (r < 0) {
            continue;
        }

        if (ev.revents & POLLPRI) {
            lseek(fd, 0, SEEK_SET);

            while ((r = read(fd, buf, sizeof(buf))) > 0) {
                write(STDOUT_FILENO, buf, r);
            }
        }
    }

    close(fd);
    return 0;
}
