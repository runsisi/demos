#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <net/if.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PROG_NAME					"igmpreport"

#define DEFAULT_IGMP_VER			3
#define DEFAULT_IGMP_GROUP          "239.0.0.33"
#define DEFAULT_REPORT_INTERVAL		10	// 10s
#define DEFAULT_RESP_INTERVAL		50	// 50/10 = 5s


struct igmpv3 {
    struct igmp v2;
    uint8_t record_type;
    uint8_t aux_data_len;
    uint16_t num_src;
    struct in_addr multicast_address;
};

static struct {
    unsigned int igmp_version;        /* IGMP Version */
    char *if_name;                    /* Network interface for binding */
    unsigned int report_interval;     /* IGMP query interval */
    struct in_addr group;             /* IGMP group address */
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
    fprintf(stderr, "    -v version     : Specify IGMP version. Default: %d\n", DEFAULT_IGMP_VER);
    fprintf(stderr, "    -g group       : Specify IGMP group address. Default: %s\n", DEFAULT_IGMP_GROUP);
    fprintf(stderr, "    -r interval    : Set membership report interval in seconds. Default: %d\n",
        DEFAULT_REPORT_INTERVAL);
}

struct ipra {
    struct ip ip;
    uint32_t ra;
};

int main(int argc, char **argv) {
    int opt;

    args.igmp_version = DEFAULT_IGMP_VER;
    args.report_interval = DEFAULT_REPORT_INTERVAL;

    while ((opt = getopt(argc, argv, "hv:i:r:g:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        case 'i':
            args.if_name = strdup(optarg);
            break;
        case 'v':
            args.igmp_version = atoi(optarg);
            break;
        case 'g': {
            struct in_addr addr;    // group address
            if (inet_aton(optarg, &addr) == 0) {
                fprintf(stderr, "malformed multicast address: %s\n", optarg);
                exit(1);
            }
            if (!IN_MULTICAST(ntohl(addr.s_addr))) {
                fprintf(stderr, "illegal multicast address: %s\n", optarg);
                exit(1);
            }
            args.group.s_addr = addr.s_addr;
            break;
        }
        case 'r':
            args.report_interval = atoi(optarg);
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

    if (args.igmp_version > 3) {
        fprintf(stderr, "Invalid IGMP version!\n");
        exit(EXIT_FAILURE);
    }

    if (args.igmp_version == 0) {
        args.igmp_version = DEFAULT_IGMP_VER;
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

    int pkt_len = 24, igmp_len = 8;
    if (args.igmp_version == 3) {
        igmp_len += 8;
    }
    pkt_len += igmp_len;

    struct ipra iphdr;
    memset(&iphdr, 0, sizeof(iphdr));

    struct in_addr igmp_group;
    inet_aton("224.0.0.22", &igmp_group);

#define iphdr iphdr.ip
    // IP v4
    iphdr.ip_v = 4;
    iphdr.ip_hl = 24 / 4;
    iphdr.ip_tos = IPTOS_CLASS_DEFAULT;
    iphdr.ip_len = htonl(pkt_len);
    iphdr.ip_id = htons(0);
    iphdr.ip_off = htons(IP_DF);
    iphdr.ip_ttl = 1;
    iphdr.ip_p = IPPROTO_IGMP;
    iphdr.ip_src = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
    iphdr.ip_dst.s_addr = args.group.s_addr;
    if (args.igmp_version == 3) {
        iphdr.ip_dst.s_addr = igmp_group.s_addr;
    }
#undef iphdr
    iphdr.ra = htonl(0x94040000);
    iphdr.ip.ip_sum = htons(checksum((uint16_t *)&iphdr, 24));

    // IGMP v1
    struct igmpv3 igmpv3hdr;
    memset(&igmpv3hdr, 0, sizeof(igmpv3hdr));
    igmpv3hdr.v2.igmp_type = IGMP_V1_MEMBERSHIP_REPORT;
    igmpv3hdr.v2.igmp_code = 0; // reserved
    igmpv3hdr.v2.igmp_cksum = 0;
    igmpv3hdr.v2.igmp_group.s_addr = args.group.s_addr;

    // IGMP v2
    if (args.igmp_version == 2) {
        igmpv3hdr.v2.igmp_type = IGMP_V2_MEMBERSHIP_REPORT;
    }

    // IGMP v3
    if (args.igmp_version == 3) {
        igmpv3hdr.v2.igmp_type = 0x22;
        igmpv3hdr.v2.igmp_group.s_addr = htonl(0x00000001); // reserved + num group records
        igmpv3hdr.record_type = 4;  // exclude
        igmpv3hdr.aux_data_len = 0;
        igmpv3hdr.num_src = 0;
        igmpv3hdr.multicast_address.s_addr = args.group.s_addr;
    }

    // ip length + igmp v1/v2 length + igmp v3 specific length
    char buffer[24 + 8 + 8] = {0};
    memcpy(buffer, &iphdr, 24);
    memcpy(buffer + 24, &igmpv3hdr, igmp_len);

    // checksum the whole packet
    igmpv3hdr.v2.igmp_cksum = htons(checksum((uint16_t *)buffer, pkt_len));
    memcpy(buffer + 24, &igmpv3hdr, igmp_len);

    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_addr.s_addr = args.group.s_addr;
    if (args.igmp_version == 3) {
        dst.sin_addr.s_addr = igmp_group.s_addr;
    }

    while (!stopped) {
        if (sendto(fd, buffer, pkt_len, 0, (struct sockaddr *)&dst,
            (socklen_t)sizeof(struct sockaddr)) < 0) {
            fprintf(stderr, "sendto() failed: %s\n", strerror(errno));
            break;
        }

        sleep(args.report_interval);
    }

    close(fd);
    free(args.if_name);

    return 0;
}
