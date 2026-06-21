#include "viewport.h"

#include <string.h>

static void sanitize_char(char *ch) {
    if (*ch < 0x20 || *ch == 0x7F) {
        *ch = ' ';
    }
}

static void copy_row_slice(char *dst, const char *src, u16 col_start, u8 width) {
    const size_t len = strlen(src);
    u8 i = 0;
    for (; i < width; ++i) {
        const u16 idx = col_start + i;
        char ch = (idx < len) ? src[idx] : ' ';
        sanitize_char(&ch);
        dst[i] = ch;
    }
    dst[i] = '\0';
}

static u16 col_window_start_live(u8 cursor_col, u8 line_len) {
    if (line_len <= OLEDTTY_VISIBLE_COLS) {
        return 0;
    }
    const i32 ideal = (i32)cursor_col - (i32)(OLEDTTY_VISIBLE_COLS - 1);
    if (ideal < 0) {
        return 0;
    }
    const i32 max_start = (i32)line_len - (i32)OLEDTTY_VISIBLE_COLS;
    return (u16)((ideal > max_start) ? max_start : ideal);
}

static void fill_term_row(viewport_row_t *vr,
                          const terminal_t *term,
                          u8 term_row,
                          u16 col_start) {
    char row[OLEDTTY_MAX_TERM_COLS + 1];
    u8 c;

    for (c = 0; c < term->cols && c < OLEDTTY_MAX_TERM_COLS; ++c) {
        row[c] = terminal_char_at(term, term_row, c);
        sanitize_char(&row[c]);
    }
    row[c] = '\0';

    copy_row_slice(vr->text, row, col_start, OLEDTTY_VISIBLE_COLS);
}

static void fill_history_row(viewport_row_t *vr,
                             const hist_entry_t *entry,
                             u16 col_start) {
    if (entry == NULL) {
        memset(vr->text, ' ', OLEDTTY_VISIBLE_COLS);
        vr->text[OLEDTTY_VISIBLE_COLS] = '\0';
        return;
    }
    copy_row_slice(vr->text, entry->text, col_start, OLEDTTY_VISIBLE_COLS);
}

static void blank_row(viewport_row_t *vr) {
    memset(vr->text, ' ', OLEDTTY_VISIBLE_COLS);
    vr->text[OLEDTTY_VISIBLE_COLS] = '\0';
}

u16 viewport_history_count(const history_t *hist) {
    return history_count(hist);
}

void view_state_init(view_state_t *vs) {
    if (vs == NULL) {
        return;
    }
    memset(vs, 0, sizeof(*vs));
    vs->mode = VIEW_MODE_LIVE;
}

void view_state_enter_live(view_state_t *vs) {
    if (vs == NULL) {
        return;
    }
    vs->mode = VIEW_MODE_LIVE;
    vs->pan_row = 0;
    vs->pan_col = 0;
}

void view_state_pan(view_state_t *vs, control_cmd_t cmd, u16 history_count) {
    view_state_apply_cmd(vs, cmd, history_count);
}

void view_state_apply_cmd(view_state_t *vs, control_cmd_t cmd, u16 history_count) {
    if (vs == NULL || cmd == CTRL_CMD_NONE) {
        return;
    }

    if (cmd == CTRL_CMD_LIVE) {
        view_state_enter_live(vs);
        return;
    }

    vs->mode = VIEW_MODE_MANUAL;

    switch (cmd) {
        case CTRL_CMD_UP:
            if (vs->pan_row + 1U < history_count + OLEDTTY_MAX_TERM_ROWS) {
                ++vs->pan_row;
            }
            break;
        case CTRL_CMD_DOWN:
            if (vs->pan_row > 0) {
                --vs->pan_row;
            }
            break;
        case CTRL_CMD_LEFT:
            if (vs->pan_col > 0) {
                --vs->pan_col;
            }
            break;
        case CTRL_CMD_RIGHT:
            ++vs->pan_col;
            break;
        default:
            break;
    }
}

void viewport_build(const terminal_t *term,
                    const history_t *hist,
                    const view_state_t *vs,
                    viewport_t *out) {
    if (term == NULL || vs == NULL || out == NULL) {
        return;
    }

    memset(out, 0, sizeof(*out));
    out->cursor_row = OLEDTTY_CURSOR_NOT_VISIBLE;
    out->cursor_col = OLEDTTY_CURSOR_NOT_VISIBLE;

    if (term->rows == 0 || term->cols == 0) {
        for (u8 vis = 0; vis < OLEDTTY_VISIBLE_ROWS; ++vis) {
            blank_row(&out->rows[vis]);
        }
        return;
    }

    const i32 end_row = (i32)term->cursor_y - (i32)vs->pan_row;
    const i32 start_row = end_row - (i32)(OLEDTTY_VISIBLE_ROWS - 1);

    u16 col_start = vs->pan_col;
    if (vs->mode == VIEW_MODE_LIVE && vs->pan_row == 0) {
        col_start = col_window_start_live(term->cursor_x, term->cols);
    }

    for (u8 vis = 0; vis < OLEDTTY_VISIBLE_ROWS; ++vis) {
        const i32 tr = start_row + (i32)vis;

        if (tr >= 0 && tr < (i32)term->rows) {
            fill_term_row(&out->rows[vis], term, (u8)tr, col_start);

            if ((u8)tr == term->cursor_y &&
                vs->mode == VIEW_MODE_LIVE &&
                vs->pan_row == 0) {
                out->cursor_row = vis;
                if (term->cursor_x >= col_start &&
                    term->cursor_x < col_start + OLEDTTY_VISIBLE_COLS) {
                    out->cursor_col = (u8)(term->cursor_x - col_start);
                }
            }
        } else if (tr < 0) {
            const u16 hist_idx = (u16)(-(tr + 1));
            fill_history_row(&out->rows[vis],
                             history_newest_at(hist, hist_idx),
                             col_start);
        } else {
            blank_row(&out->rows[vis]);
        }
    }
}
