#include "keyboard.h"

#include "log.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static bool key_is_ctrl(int code) {
    return code == KEY_LEFTCTRL || code == KEY_RIGHTCTRL;
}

static bool key_is_shift(int code) {
    return code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT;
}

static bool key_is_vertical_arrow(int code) {
    return code == KEY_UP || code == KEY_DOWN;
}

static bool key_is_horizontal_arrow(int code) {
    return code == KEY_LEFT || code == KEY_RIGHT;
}

static control_cmd_t arrow_to_cmd(int code) {
    switch (code) {
        case KEY_UP:    return CTRL_CMD_UP;
        case KEY_DOWN:  return CTRL_CMD_DOWN;
        case KEY_LEFT:  return CTRL_CMD_LEFT;
        case KEY_RIGHT: return CTRL_CMD_RIGHT;
        default:        return CTRL_CMD_NONE;
    }
}

static int try_open_keyboard(const char *path) {
    const int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    unsigned long evbit[(EV_MAX + 1) / (8U * sizeof(unsigned long)) + 1U];
    unsigned long keybit[(KEY_MAX + 1) / (8U * sizeof(unsigned long)) + 1U];

    memset(evbit, 0, sizeof(evbit));
    memset(keybit, 0, sizeof(keybit));

    if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) {
        close(fd);
        return -1;
    }
    if ((evbit[EV_KEY / (8U * sizeof(unsigned long))] &
         (1UL << (EV_KEY % (8U * sizeof(unsigned long))))) == 0) {
        close(fd);
        return -1;
    }

    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) {
        close(fd);
        return -1;
    }
    if ((keybit[KEY_A / (8U * sizeof(unsigned long))] &
         (1UL << (KEY_A % (8U * sizeof(unsigned long))))) == 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static int autodetect_keyboard(void) {
    DIR *dir = opendir("/dev/input");
    if (dir == NULL) {
        return -1;
    }

    struct dirent *ent;
    char path[128];
    int found = -1;

    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "event", 5) != 0) {
            continue;
        }
        snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);
        found = try_open_keyboard(path);
        if (found >= 0) {
            log_info("keyboard: using %s", path);
            break;
        }
    }

    closedir(dir);
    return found;
}

int keyboard_open(keyboard_t *kb, const char *device_path) {
    if (kb == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(kb, 0, sizeof(*kb));
    kb->fd = -1;

    if (device_path != NULL && device_path[0] != '\0') {
        kb->fd = try_open_keyboard(device_path);
        if (kb->fd < 0) {
            return -1;
        }
        log_info("keyboard: using %s", device_path);
        return 0;
    }

    kb->fd = autodetect_keyboard();
    if (kb->fd < 0) {
        log_debug("keyboard: no input device found");
        return -1;
    }

    return 0;
}

void keyboard_close(keyboard_t *kb) {
    if (kb == NULL) {
        return;
    }
    if (kb->fd >= 0) {
        close(kb->fd);
        kb->fd = -1;
    }
}

control_cmd_t keyboard_poll(keyboard_t *kb) {
    if (kb == NULL || kb->fd < 0) {
        return CTRL_CMD_NONE;
    }

    control_cmd_t result = CTRL_CMD_NONE;
    struct input_event ev;

    for (;;) {
        const ssize_t n = read(kb->fd, &ev, sizeof(ev));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            log_debug("keyboard read failed: %s", strerror(errno));
            break;
        }
        if ((size_t)n < sizeof(ev)) {
            break;
        }

        if (ev.type != EV_KEY) {
            continue;
        }

        if (key_is_ctrl(ev.code)) {
            if (ev.code == KEY_LEFTCTRL) {
                kb->ctrl_left = (ev.value != 0);
            } else {
                kb->ctrl_right = (ev.value != 0);
            }
            continue;
        }

        if (key_is_shift(ev.code)) {
            if (ev.code == KEY_LEFTSHIFT) {
                kb->shift_left = (ev.value != 0);
            } else {
                kb->shift_right = (ev.value != 0);
            }
            continue;
        }

        if (ev.value != 1) {
            continue;
        }

        const bool ctrl = kb->ctrl_left || kb->ctrl_right;
        const bool shift = kb->shift_left || kb->shift_right;

        if (ctrl && shift && key_is_vertical_arrow(ev.code)) {
            result = arrow_to_cmd(ev.code);
        } else if (ctrl && key_is_horizontal_arrow(ev.code)) {
            result = arrow_to_cmd(ev.code);
        }
    }

    return result;
}
