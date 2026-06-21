#include "control.h"
#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static control_cmd_t parse_command(const char *buf, ssize_t len) {
    char cmd[32];
    size_t n = (size_t)len;
    if (n >= sizeof(cmd)) {
        n = sizeof(cmd) - 1U;
    }
    memcpy(cmd, buf, n);
    cmd[n] = '\0';

    for (char *p = cmd; *p != '\0'; ++p) {
        if (*p == '\n' || *p == '\r') {
            *p = '\0';
            break;
        }
    }

    if (strcmp(cmd, "up") == 0) {
        return CTRL_CMD_UP;
    }
    if (strcmp(cmd, "down") == 0) {
        return CTRL_CMD_DOWN;
    }
    if (strcmp(cmd, "left") == 0) {
        return CTRL_CMD_LEFT;
    }
    if (strcmp(cmd, "right") == 0) {
        return CTRL_CMD_RIGHT;
    }
    if (strcmp(cmd, "live") == 0) {
        return CTRL_CMD_LIVE;
    }
    return CTRL_CMD_NONE;
}

int control_server_start(control_server_t *srv, const char *socket_path) {
    if (srv == NULL || socket_path == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(srv, 0, sizeof(*srv));
    srv->listen_fd = -1;
    srv->client_fd = -1;

    srv->listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (srv->listen_fd < 0) {
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socket_path);

    unlink(socket_path);

    if (bind(srv->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(srv->listen_fd);
        srv->listen_fd = -1;
        return -1;
    }

    chmod(socket_path, 0666);

    if (listen(srv->listen_fd, 4) < 0) {
        close(srv->listen_fd);
        srv->listen_fd = -1;
        unlink(socket_path);
        return -1;
    }

    return 0;
}

void control_server_stop(control_server_t *srv) {
    if (srv == NULL) {
        return;
    }
    if (srv->client_fd >= 0) {
        close(srv->client_fd);
        srv->client_fd = -1;
    }
    if (srv->listen_fd >= 0) {
        close(srv->listen_fd);
        srv->listen_fd = -1;
    }
    unlink(OLEDTTY_CONTROL_SOCKET);
}

control_cmd_t control_server_poll(control_server_t *srv) {
    if (srv == NULL || srv->listen_fd < 0) {
        return CTRL_CMD_NONE;
    }

    if (srv->client_fd < 0) {
        srv->client_fd = accept4(srv->listen_fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (srv->client_fd < 0) {
            return CTRL_CMD_NONE;
        }
    }

    char buf[64];
    const ssize_t n = read(srv->client_fd, buf, sizeof(buf));
    if (n <= 0) {
        close(srv->client_fd);
        srv->client_fd = -1;
        return CTRL_CMD_NONE;
    }

    const control_cmd_t cmd = parse_command(buf, n);
    close(srv->client_fd);
    srv->client_fd = -1;
    return cmd;
}
