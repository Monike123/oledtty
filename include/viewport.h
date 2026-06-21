#ifndef OLEDTTY_VIEWPORT_H
#define OLEDTTY_VIEWPORT_H

#include "types.h"
#include "config.h"
#include "terminal.h"
#include "history.h"
#include "control.h"

typedef enum {
    VIEW_MODE_LIVE = 0,
    VIEW_MODE_MANUAL
} view_mode_t;

typedef struct {
    view_mode_t mode;
    u16 pan_row;
    u16 pan_col;
} view_state_t;

typedef struct {
    char text[OLEDTTY_VISIBLE_COLS + 1];
} viewport_row_t;

typedef struct {
    viewport_row_t rows[OLEDTTY_VISIBLE_ROWS];
    u8  cursor_row;
    u8  cursor_col;
} viewport_t;

#define OLEDTTY_CURSOR_NOT_VISIBLE 0xFFU

void view_state_init(view_state_t *vs);
void view_state_enter_live(view_state_t *vs);
void view_state_apply_cmd(view_state_t *vs, control_cmd_t cmd, u16 history_count);
void view_state_pan(view_state_t *vs, control_cmd_t cmd, u16 history_count);

void viewport_build(const terminal_t *term,
                    const history_t *hist,
                    const view_state_t *vs,
                    viewport_t *out);

u16 viewport_history_count(const history_t *hist);

#endif
