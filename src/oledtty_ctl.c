#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s <up|down|left|right|live>\n"
            "  Control oledtty browse mode over %s\n",
            prog, OLEDTTY_CONTROL_SOCKET);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", OLEDTTY_CONTROL_SOCKET);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    char msg[32];
    snprintf(msg, sizeof(msg), "%s\n", argv[1]);
    if (write(fd, msg, strlen(msg)) < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
