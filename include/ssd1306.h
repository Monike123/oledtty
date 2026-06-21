#ifndef OLEDTTY_SSD1306_H
#define OLEDTTY_SSD1306_H

#include "types.h"
#include "i2c.h"
#include "config.h"

typedef struct {
    i2c_bus_t bus;
    u8 width;
    u8 height;
    u8 pages;
} ssd1306_t;

int  ssd1306_init(ssd1306_t *dev, const char *i2c_path, u8 address);
void ssd1306_shutdown(ssd1306_t *dev);
int  ssd1306_send_commands(ssd1306_t *dev, const u8 *cmds, u16 len);
int  ssd1306_flush_frame(ssd1306_t *dev, const u8 *framebuffer, u16 len);
int  ssd1306_flush_pages(ssd1306_t *dev, const u8 *framebuffer, u8 page_mask);
int  ssd1306_set_contrast(ssd1306_t *dev, u8 contrast);
int  ssd1306_clear_display(ssd1306_t *dev);

#endif
