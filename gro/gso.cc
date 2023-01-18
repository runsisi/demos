#include <iostream>
#include <stdio.h>
#include <string.h>

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

using namespace std;

const int MSS = 1460;
const char *server_addr = "10.0.0.60";
const short server_port = 3333;

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) {
        int err = errno;
        printf("socket failed: %s\n", ::strerror(err));
        exit(1);
    }

    int gso_len = MSS;
    int r = setsockopt(fd, SOL_UDP, UDP_SEGMENT, &gso_len, sizeof(gso_len));
    if (r < 0) {
        int err = errno;
        printf("setsockopt failed: %s\n", ::strerror(err));
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server_addr);
    addr.sin_port = htons(server_port);

    int BUF_SIZE = MSS * 4 + 1;
    char *buf = new char[BUF_SIZE];
    strcpy(buf, "abcdefghijk");
    r = sendto(fd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (r < 0) {
        int err = errno;
        printf("sendto failed: %s\n", ::strerror(err));
        exit(1);
    }

    close(fd);

    return 0;
}
