#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#ifndef UDP_SEGMENT
#define UDP_SEGMENT 103
#endif

#ifndef UDP_GRO
#define UDP_GRO 104
#endif

const int MSS = 1460;
const char *server_addr = "10.0.0.61";
const short server_port = 3333;

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) {
        int err = errno;
        printf("socket failed: %s\n", ::strerror(err));
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server_addr);
    addr.sin_port = htons(server_port);
    int r = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (r < 0) {
        int err = errno;
        printf("bind failed: %s\n", ::strerror(err));
        exit(1);
    }

    int val = 1;
    r = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
    if (r < 0) {
        int err = errno;
        printf("setsockopt SO_REUSEPORT failed: %s\n", ::strerror(err));
        exit(1);
    }

    // fraglist gro needs UDP_GRO option turned off
//    r = setsockopt(fd, SOL_UDP, UDP_GRO, &val, sizeof(val));
//    if (r < 0) {
//        int err = errno;
//        printf("setsockopt UDP_GRO failed: %s\n", ::strerror(err));
//        exit(1);
//    }

    int BUF_SIZE = MSS * 5;
    char *buf = new char[BUF_SIZE];
//    int addrlen = 0;
//    while (true) {
//        r = recvfrom(fd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
//        if (r < 0) {
//            int err = errno;
//            printf("recvfrom failed: %s\n", ::strerror(err));
//            exit(1);
//        }
//
//        printf("received %d bytes\n", r);
//    }

    char ctl[CMSG_SPACE(sizeof (uint16_t))] = {0};
    struct msghdr msg = {0};
    struct iovec iov = {0};

    iov.iov_base = buf;
    iov.iov_len = BUF_SIZE;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctl;
    msg.msg_controllen = sizeof(ctl);

    while (true) {
        r = recvmsg(fd, &msg, 0);
        if (r < 0) {
            int err = errno;
            printf("recvmsg failed: %s\n", ::strerror(err));
            exit(1);
        }

        printf("received %d bytes\n", r);

        // only valid for UDP_GRO
        for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_UDP && cmsg->cmsg_type == UDP_GRO) {
                uint16_t gro_size = *(uint16_t *)CMSG_DATA(cmsg);
                printf("gro_size = %d\n", gro_size);
                break;
            }
        }
    }

    close(fd);
    return 0;
}
