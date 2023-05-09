#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PROG_NAME					"ipopt"

#define DEFAULT_INTERVAL            1   // 1s

static struct {
    char *if_name;                  // network interface for binding
    unsigned int interval;          // send interval
    struct in_addr addr;            // ping address
    uint8_t option;                 // IP option
} args;

atomic_int stopped = 0;

static void signal_handler(int signal) {
    if (signal == SIGTERM) {
        stopped = 1;
    }
}

/* Calculate checksum of array of 2-byte words in network order (big-endian).
 * Returns value in host's byte order. */
uint16_t checksum(uint16_t *addr, int len) {
    uint32_t sum = 0;

    // Sum of each 2-byte values
    while (len > 1) {
        sum += ntohs(*(addr++));
        len -= 2;
    }

    // Add partial block if available at the end of data
    if (len > 0) {
        sum += *(uint8_t *)addr;
    }

    // Sum lower 16 bits and upper 16 bits
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    // Return one's compliment of sum.
    return (uint16_t)(~sum);
}

void print_usage(void) {
    fprintf(stderr, "Usage: %s [options]\n", PROG_NAME);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -h             : Print help message.\n");
    fprintf(stderr, "    -i interface   : Specify network interface.\n");
    fprintf(stderr, "    -a address     : Specify ping address.\n");
    fprintf(stderr, "    -r interval    : Set ping interval in seconds. Default: %d.\n", DEFAULT_INTERVAL);
    fprintf(stderr, "    -o option      : IP option (0/1/136/148). Default: None.\n");
}

int main(int argc, char **argv) {
    int opt;

    args.interval = DEFAULT_INTERVAL;
    args.option = (uint8_t)-1;

    while ((opt = getopt(argc, argv, "hi:r:a:o:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        case 'i':
            args.if_name = strdup(optarg);
            break;
        case 'a': {
            struct in_addr addr;    // ping address
            if (inet_aton(optarg, &addr) == 0) {
                fprintf(stderr, "malformed ping address: %s\n", optarg);
                exit(1);
            }
            args.addr.s_addr = addr.s_addr;
            break;
        }
        case 'r':
            args.interval = atoi(optarg);
            break;
        case 'o':
            args.option = atoi(optarg);
            if (args.option != 0 && args.option != 1 && args.option != 136 && args.option != 148) {
                fprintf(stderr, "not supported IP option: %s\n", optarg);
                exit(1);
            }
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    if (!args.if_name || strlen(args.if_name) == 0) {
        fprintf(stderr, "Interface argument missing!\n");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sigset_t sig_mask;
    sigemptyset(&sig_mask);
    sigaddset(&sig_mask, SIGTERM);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sa.sa_mask = sig_mask;

    if (sigaction(SIGTERM, &sa, 0) == -1) {
        fprintf(stderr, "sigaction() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int fd;
    if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP)) < 0) {
        fprintf(stderr, "socket() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int val = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val)) < 0) {
        fprintf(stderr, "setsockopt() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, args.if_name, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "ioctl() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        fprintf(stderr, "setsockopt() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, args.if_name, sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        fprintf(stderr, "Failed to get IP address of interface: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    srand(time(0));

    uint16_t id = random();
    uint16_t seq = 1;

    int ip_hlen = 20;
    if (args.option != (uint8_t)-1) {
        // EOOL, NOP, SID, RTRALT
        ip_hlen += 4;
    }

    // append icmp echo w/o data
    uint16_t ip_len = ip_hlen + 8;

    char *pkt = malloc(ip_len);
    memset(pkt, 0, ip_len);

    do {
        struct ip *iphdr = (struct ip *)pkt;

        // IP v4
        iphdr->ip_v = 4;
        iphdr->ip_hl = ip_hlen / 4;
        iphdr->ip_tos = IPTOS_CLASS_DEFAULT;
        iphdr->ip_len = htonl(ip_len);
        iphdr->ip_id = htons(0);
        iphdr->ip_off = htons(IP_DF);
        iphdr->ip_ttl = 1;
        iphdr->ip_p = IPPROTO_ICMP;
        iphdr->ip_src = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
        iphdr->ip_dst.s_addr = args.addr.s_addr;
        // ip option
        char *opt = (char *)(iphdr + 1);
        if (args.option == 1) { // NOP
            opt[0] = 1;
        } else if (args.option == 136) { // SID
            *(uint32_t *)opt = htonl(136 << 24 | 4 << 16 | 0);
        } else if (args.option == 148) { // RTRALT
            *(uint32_t *)opt = htonl(148 << 24 | 4 << 16 | 0);
        }
        iphdr->ip_sum = htons(checksum((uint16_t *)&iphdr, 20));

        struct icmphdr *icmp = (struct icmphdr *)((char *)iphdr + ip_hlen);
        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->checksum = 0;
        icmp->un.echo.id = 0;
        icmp->un.echo.sequence = seq++;

        // checksum the icmp header & data
        icmp->checksum = htons(checksum((uint16_t *)icmp, 8));

        struct sockaddr_in dst;
        memset(&dst, 0, sizeof(dst));
        dst.sin_addr.s_addr = args.addr.s_addr;

        if (sendto(fd, pkt, ip_len, 0, (struct sockaddr *)&dst,
            (socklen_t)sizeof(struct sockaddr)) < 0) {
            fprintf(stderr, "sendto() failed: %s\n", strerror(errno));
            break;
        }

        sleep(args.interval);
    } while (!stopped);

    close(fd);
    free(args.if_name);

    return 0;
}
