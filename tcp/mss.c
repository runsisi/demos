#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define PORT 8101

struct args {
    struct in_addr addr;
    int port;
    int server;
};

static void usage() {
    const char *usage_text =
            "TCP mss demo.\n\n"
            "%s options address\n\n"
            "  -h, --help    Print this help\n"
            "  -p, --port    TCP port (default 8101)\n"
            "  -s, --server  Running in server mode\n\n";
    printf(usage_text, "udp-cast");
}

static void parse_args(int argc, char **argv, struct args *args) {
    char *short_opts = "hp:s";
    struct option long_opts[] = {
        {"help",   no_argument,       NULL, 'h'},
        {"port",   required_argument, NULL, 'p'},
        {"server", no_argument,       NULL, 's'},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
        switch (opt) {
        case 'p': {
            int port = atoi(optarg);
            if (port < 1 || port > 65535) {
                fprintf(stderr, "invalid port: %s\n", optarg);
                exit(1);
            }
            args->port = htons(port);
            break;
        }
        case 's': {
            args->server = 1;
            break;
        }
        case 'h': {
            usage();
            exit(0);
        }
        default: {
            usage();
            exit(1);
        }
        }
    }

    // handle positional args
    if (args->server) {
        args->addr.s_addr = htonl(INADDR_ANY);
    } else {
        struct in_addr addr;    // target address
        if (inet_aton(argv[optind++], &addr) == 0) {
            fprintf(stderr, "malformed target address: %s\n", optarg);
            exit(1);
        }
        args->addr.s_addr = addr.s_addr;
    }

    // handle default args
    if (args->port == 0) {
        args->port = htons(PORT);
    }
}

int g_running = 1;

static void term_handler(int sig) {
    g_running = 0;
    fprintf(stdout, "exiting!\n");
}

int main(int argc, char **argv) {
    int r;
    struct sigaction act;
    act.sa_handler = term_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    r = sigaction(SIGINT, &act, NULL);
    if (r < 0) {
        r = errno;
        fprintf(stderr, "register SIGINT handler failed: %s\n", strerror(r));
        return 1;
    }
    r = sigaction(SIGTERM, &act, NULL);
    if (r < 0) {
        r = errno;
        fprintf(stderr, "register SIGTERM handler failed: %s\n", strerror(r));
        return 1;
    }

    struct args args;
    memset(&args, 0, sizeof(args));
    parse_args(argc, argv, &args);

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (fd < 0) {
        r = errno;
        fprintf(stderr, "create socket failed: %s\n", strerror(r));
        return 1;
    }

    int opt_reuseaddr = 1;
    r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt_reuseaddr, sizeof(opt_reuseaddr));
    if (r < 0) {
        r = errno;
        fprintf(stderr, "set socket option \"SO_REUSEADDR\" failed: %s\n", strerror(r));
        close(fd);
        return 1;
    }

    if (args.server) {
        struct sockaddr_in baddr = {
            .sin_family = AF_INET,
            .sin_addr = args.addr,
            .sin_port = args.port,
        };

        // do not bind to INADDR_ANY even if in bcast mode, since we don't want to receive
        // all packets in bcast mode
        r = bind(fd, (struct sockaddr *)&baddr, sizeof(baddr));
        if (r < 0) {
            r = errno;
            fprintf(stderr, "bind socket failed: %s\n", strerror(r));
            close(fd);
            return 1;
        }

        r = listen(fd, 10);
        if (r < 0) {
            r = errno;
            fprintf(stderr, "listen failed: %s\n", strerror(r));
            close(fd);
            return 1;
        }

        while (g_running) {
            r = accept(fd, NULL, NULL);
            if (r < 0) {
                r = errno;
                fprintf(stderr, "accept failed: %s\n", strerror(r));
                close(fd);
                return 1;
            }

            int nfd = r;
            int val = 0;
            int len = sizeof(val);
            r = getsockopt(nfd, IPPROTO_TCP, TCP_MAXSEG, &val, &len);
            if (r < 0) {
                r = errno;
                fprintf(stderr, "getsockopt(TCP_MAXSEG) failed: %s\n", strerror(r));
            } else {
                printf("mss: %d\n", val);
            }
            close(nfd);
        }
    } else {
        struct sockaddr_in taddr = {
            .sin_family = AF_INET,
            .sin_addr = args.addr,
            .sin_port = args.port,
        };

        r = connect(fd, (struct sockaddr *)&taddr, sizeof(taddr));
        if (r < 0) {
            r = errno;
            fprintf(stderr, "connect failed: %s\n", strerror(r));
            close(fd);
            return 1;
        }

        int val = 0;
        int len = sizeof(val);
        r = getsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &val, &len);
        if (r < 0) {
            r = errno;
            fprintf(stderr, "getsockopt(TCP_MAXSEG) failed: %s\n", strerror(r));
        } else {
            printf("mss: %d\n", val);
        }
    }

    close(fd);
    return 0;
}
