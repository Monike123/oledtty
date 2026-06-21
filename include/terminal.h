#ifndef OLEDTTY_TERMINAL_H
#define OLEDTTY_TERMINAL_H

#include "types.h"
#include "config.h"

typedef struct {
    int fd;
    char path[64];

    u8 rows;
    u8 cols;
    u8 cursor_x;
    u8 cursor_y;

    char cells[OLEDTTY_MAX_TERM_ROWS][OLEDTTY_MAX_TERM_COLS];
} terminal_t;

int  terminal_open(terminal_t *term, const char *vcsa_path);
int  terminal_read(terminal_t *term);
void terminal_close(terminal_t *term);
char terminal_char_at(const terminal_t *term, u8 row, u8 col);

#endif
