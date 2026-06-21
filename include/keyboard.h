#ifndef OLEDTTY_KEYBOARD_H
#define OLEDTTY_KEYBOARD_H

#include "types.h"
#include "control.h"

typedef struct {
    int fd;
    bool ctrl_left;
    bool ctrl_right;
    bool shift_left;
    bool shift_right;
} keyboard_t;

int  keyboard_open(keyboard_t *kb, const char *device_path);
void keyboard_close(keyboard_t *kb);
control_cmd_t keyboard_poll(keyboard_t *kb);

#endif
