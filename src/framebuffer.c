#include "framebuffer.h"
#include "font5x7.h"

#include <string.h>

void fb_init(framebuffer_t *fb) {
    if (fb == NULL) {
        return;
    }
    fb->width = OLEDTTY_WIDTH;
    fb->height = OLEDTTY_HEIGHT;
    fb->pages = OLEDTTY_PAGES;
    fb_clear(fb);
}

void fb_clear(framebuffer_t *fb) {
    if (fb == NULL) {
        return;
    }
    memset(fb->data, 0, sizeof(fb->data));
}

void fb_set_pixel(framebuffer_t *fb, i16 x, i16 y, bool on) {
    if (fb == NULL) {
        return;
    }

    if (x < 0 || y < 0 || x >= fb->width || y >= fb->height) {
        return;
    }

    const u16 byte_index = (u16)(x + (y / 8) * fb->width);
    const u8 bit_mask = (u8)(1U << (y & 7));

    if (on) {
        fb->data[byte_index] |= bit_mask;
    } else {
        fb->data[byte_index] &= (u8)~bit_mask;
    }
}

void fb_invert_rect(framebuffer_t *fb, i16 x, i16 y, i16 w, i16 h) {
    if (fb == NULL || w <= 0 || h <= 0) {
        return;
    }

    for (i16 yy = 0; yy < h; ++yy) {
        for (i16 xx = 0; xx < w; ++xx) {
            const i16 px = x + xx;
            const i16 py = y + yy;
            if (px < 0 || py < 0 || px >= fb->width || py >= fb->height) {
                continue;
            }
            const u16 byte_index = (u16)(px + (py / 8) * fb->width);
            const u8 bit_mask = (u8)(1U << (py & 7));
            fb->data[byte_index] ^= bit_mask;
        }
    }
}

void fb_draw_hline(framebuffer_t *fb, i16 x, i16 y, i16 w, bool on) {
    if (fb == NULL || w <= 0) {
        return;
    }

    for (i16 i = 0; i < w; ++i) {
        fb_set_pixel(fb, (i16)(x + i), y, on);
    }
}

void fb_copy(framebuffer_t *dst, const framebuffer_t *src) {
    if (dst == NULL || src == NULL) {
        return;
    }
    memcpy(dst->data, src->data, sizeof(dst->data));
}

u8 fb_diff_page_mask(const framebuffer_t *front, const framebuffer_t *back) {
    if (front == NULL || back == NULL) {
        return (u8)((1U << OLEDTTY_PAGES) - 1U);
    }

    u8 mask = 0;
    for (u8 page = 0; page < OLEDTTY_PAGES; ++page) {
        const size_t offset = (size_t)page * (size_t)OLEDTTY_WIDTH;
        if (memcmp(front->data + offset, back->data + offset, OLEDTTY_WIDTH) != 0) {
            mask |= (u8)(1U << page);
        }
    }
    return mask;
}

void fb_draw_char(framebuffer_t *fb, i16 x, i16 y, char ch, bool on) {
    if (fb == NULL) {
        return;
    }

    const u8 *glyph = font5x7_lookup(ch);
    if (glyph == NULL) {
        return;
    }

    for (u8 col = 0; col < OLEDTTY_FONT_W; ++col) {
        u8 bits = glyph[col];
        for (u8 row = 0; row < OLEDTTY_FONT_H; ++row) {
            if ((bits >> row) & 0x01U) {
                fb_set_pixel(fb, (i16)(x + col), (i16)(y + row), on);
            }
        }
    }
}

void fb_draw_text(framebuffer_t *fb, i16 x, i16 y, const char *text, bool on) {
    if (fb == NULL || text == NULL) {
        return;
    }

    i16 cursor_x = x;
    while (*text != '\0') {
        if (*text == '\n') {
            cursor_x = x;
            y += OLEDTTY_CHAR_CELL_H;
        } else {
            fb_draw_char(fb, cursor_x, y, *text, on);
            cursor_x += OLEDTTY_CHAR_CELL_W;
        }
        ++text;
    }
}
