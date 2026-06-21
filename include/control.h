#ifndef OLEDTTY_CONTROL_H
#define OLEDTTY_CONTROL_H

#include "types.h"

typedef enum {
    CTRL_CMD_UP = 0,
    CTRL_CMD_DOWN,
    CTRL_CMD_LEFT,
    CTRL_CMD_RIGHT,
    CTRL_CMD_LIVE,
    CTRL_CMD_NONE
} control_cmd_t;

typedef struct {
    int listen_fd;
    int client_fd;
} control_server_t;

int  control_server_start(control_server_t *srv, const char *socket_path);
void control_server_stop(control_server_t *srv);
control_cmd_t control_server_poll(control_server_t *srv);

#endif
