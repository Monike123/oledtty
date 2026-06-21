#ifndef OLEDTTY_FRAMEBUFFER_H
#define OLEDTTY_FRAMEBUFFER_H

#include "types.h"
#include "config.h"

typedef struct {
    u8 width;
    u8 height;
    u8 pages;
    u8 data[OLEDTTY_FB_SIZE];
} framebuffer_t;

void fb_init(framebuffer_t *fb);
void fb_clear(framebuffer_t *fb);
void fb_set_pixel(framebuffer_t *fb, i16 x, i16 y, bool on);
void fb_draw_hline(framebuffer_t *fb, i16 x, i16 y, i16 w, bool on);
void fb_draw_char(framebuffer_t *fb, i16 x, i16 y, char ch, bool on);
void fb_draw_text(framebuffer_t *fb, i16 x, i16 y, const char *text, bool on);
void fb_invert_rect(framebuffer_t *fb, i16 x, i16 y, i16 w, i16 h);
void fb_copy(framebuffer_t *dst, const framebuffer_t *src);
u8   fb_diff_page_mask(const framebuffer_t *front, const framebuffer_t *back);

#endif
