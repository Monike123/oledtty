#include "render.h"

void render_frame(framebuffer_t *fb,
                  const viewport_t *vp,
                  bool cursor_visible) {
    if (fb == NULL || vp == NULL) {
        return;
    }

    fb_clear(fb);

    for (u8 row = 0; row < OLEDTTY_VISIBLE_ROWS; ++row) {
        const i16 y = (i16)(OLEDTTY_TEXT_MARGIN_Y + row * OLEDTTY_CHAR_CELL_H);
        const i16 x = OLEDTTY_TEXT_MARGIN_X;

        for (u8 col = 0; col < OLEDTTY_VISIBLE_COLS; ++col) {
            char ch = vp->rows[row].text[col];
            if (ch == '\0') {
                ch = ' ';
            }
            const i16 cx = (i16)(x + (i16)col * OLEDTTY_CHAR_CELL_W);
            fb_draw_char(fb, cx, y, ch, true);
        }
    }

    if (cursor_visible &&
        vp->cursor_row != OLEDTTY_CURSOR_NOT_VISIBLE &&
        vp->cursor_col != OLEDTTY_CURSOR_NOT_VISIBLE) {
        const i16 py = (i16)(OLEDTTY_TEXT_MARGIN_Y +
                             vp->cursor_row * OLEDTTY_CHAR_CELL_H);
        const i16 px = (i16)(OLEDTTY_TEXT_MARGIN_X +
                             (i16)vp->cursor_col * OLEDTTY_CHAR_CELL_W);
        fb_invert_rect(fb, px, py, OLEDTTY_CHAR_CELL_W, OLEDTTY_CHAR_CELL_H);
    }
}
