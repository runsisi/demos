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

// IP options
enum {
    EOOL = 0,
    NOP = 1,
    SID = 136,
    RTRALT = 148,
};

#define DEFAULT_INTERVAL            1   // 1s
#define DEFAULT_DATA_LENGTH         56

static struct {
    char *if_name;                  // network interface for binding
    unsigned int interval;          // send interval
    struct in_addr addr;            // ping address
    uint8_t option;                 // IP option
    int data_len;              // ping data length
} args;

atomic_int stopped = 0;

static void signal_handler(int signal) {
    if (signal == SIGTERM) {
        stopped = 1;
    }
}

uint16_t checksum(uint16_t *addr, int len) {
    uint32_t sum = 0;

    while (len > 1) {
        sum += *addr++;
        if (sum & 0x80000000) {
            sum = (sum & 0xffff) + (sum >> 16);
        }
        len -= 2;
    }

    if (len) {
        sum += *(uint8_t *)addr;
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

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
    fprintf(stderr, "    -l length      : Ping data length. Default: %d.\n", DEFAULT_DATA_LENGTH);
}

int main(int argc, char **argv) {
    int opt;

    args.interval = DEFAULT_INTERVAL;
    args.option = (uint8_t)-1;
    args.data_len = DEFAULT_DATA_LENGTH;

    while ((opt = getopt(argc, argv, "hi:r:a:o:l:")) != -1) {
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
                exit(EXIT_FAILURE);
            }
            args.addr.s_addr = addr.s_addr;
            break;
        }
        case 'r':
            args.interval = atoi(optarg);
            break;
        case 'o':
            args.option = atoi(optarg);
            if (args.option != EOOL && args.option != NOP && args.option != SID && args.option != RTRALT) {
                fprintf(stderr, "IP option not supported: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'l': {
            // When the IP_HDRINCL option is set, datagrams will not be
            // fragmented and are limited to the interface MTU.
            args.data_len = atoi(optarg);
            // mtu - max ip header - icmp header
            int max_len = 1500 - 60 - 8;
            if (args.data_len > max_len) {
                fprintf(stderr, "exceeded max ping data length: %d\n", max_len);
                exit(EXIT_FAILURE);
            }
            break;
        }
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
    // IPPROTO_RAW implies enabled IP_HDRINCL
    if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        fprintf(stderr, "socket(SOCK_RAW) failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int val = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val)) < 0) {
        fprintf(stderr, "setsockopt(IP_HDRINCL) failed: %s\n", strerror(errno));
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
        if (args.option == EOOL || args.option == NOP) {
            ip_hlen += 1;
        } else if (args.option == SID || args.option == RTRALT) {
            ip_hlen += 4;
        }
    }

    ip_hlen = (ip_hlen + 3) & (~3);

    // append icmp echo w/ data
    uint16_t ip_len = ip_hlen + 8 + args.data_len;

    char *pkt = malloc(ip_len);
    memset(pkt, 0, ip_len);

    struct ip *iphdr = (struct ip *)pkt;
    struct icmphdr *icmphdr = (struct icmphdr *)((char *)iphdr + ip_hlen);
    char *data = (char *)icmphdr + 8;

    int iter = 0;
    for (;;) {
        for (int i = '0'; i <= 'z' && args.data_len; i++, args.data_len--) {
            data[i - '0' + iter * ('z' - '0' + 1)] = (char)i;
        }

        if (!args.data_len) {
            break;
        }
        iter++;
    }

    do {
        // IP v4
        iphdr->ip_v = 4;
        iphdr->ip_hl = ip_hlen / 4;
        iphdr->ip_tos = IPTOS_CLASS_DEFAULT;
        iphdr->ip_len = htonl(ip_len);
        iphdr->ip_id = htons(0);
        iphdr->ip_off = htons(IP_DF);
        iphdr->ip_ttl = 64;
        iphdr->ip_p = IPPROTO_ICMP;
        iphdr->ip_src = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
        iphdr->ip_dst.s_addr = args.addr.s_addr;
        // ip option
        char *opt = (char *)(iphdr + 1);
        if (args.option == NOP) { // NOP
            opt[0] = NOP;
        } else if (args.option == SID) { // SID
            *(uint32_t *)opt = htonl(SID << 24 | 4 << 16 | 0);
        } else if (args.option == RTRALT) { // RTRALT
            *(uint32_t *)opt = htonl(RTRALT << 24 | 4 << 16 | 0);
        }
        // checksum the ip header
        iphdr->ip_sum = checksum((uint16_t *)&iphdr, ip_hlen);

        icmphdr->type = ICMP_ECHO;
        icmphdr->code = 0;
        icmphdr->checksum = 0;
        icmphdr->un.echo.id = htons(id);
        icmphdr->un.echo.sequence = htons(seq++);
        // checksum the icmp header & data
        icmphdr->checksum = checksum((uint16_t *)icmphdr, ip_len - ip_hlen);

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

    free(pkt);
    close(fd);
    free(args.if_name);

    return 0;
}
