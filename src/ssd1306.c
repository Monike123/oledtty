#include "ssd1306.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

static const u8 ssd1306_init_sequence[] = {
    0xAE,       // Display OFF
    0xD5, 0x80, // Set display clock divide ratio / oscillator frequency
    0xA8, 0x3F, // Multiplex ratio 1/64
    0xD3, 0x00, // Display offset = 0
    0x40,       // Start line = 0
    0x8D, 0x14, // Charge pump enable
    0x20, 0x00, // Horizontal addressing mode
    0xA1,       // Segment remap
    0xC8,       // COM scan direction remapped
    0xDA, 0x12, // COM pins hardware configuration
    0x81, 0x7F, // Contrast
    0xD9, 0xF1, // Pre-charge period
    0xDB, 0x40, // VCOMH deselect level
    0xA4,       // Entire display ON from RAM
    0xA6,       // Normal display
    0x2E,       // Deactivate scroll
    0xAF        // Display ON
};

int ssd1306_send_commands(ssd1306_t *dev, const u8 *cmds, u16 len) {
    if (dev == NULL || cmds == NULL || len == 0) {
        errno = EINVAL;
        return -1;
    }

    return i2c_write_packet(&dev->bus, 0x00, cmds, len);
}

int ssd1306_flush_frame(ssd1306_t *dev, const u8 *framebuffer, u16 len) {
    if (dev == NULL || framebuffer == NULL || len == 0) {
        errno = EINVAL;
        return -1;
    }

    /* Delegate to page-by-page flush — safer than a single 1024-byte
     * I2C transaction which may exceed some controllers' limits. */
    return ssd1306_flush_pages(dev, framebuffer, (u8)((1U << OLEDTTY_PAGES) - 1U));
}

int ssd1306_flush_pages(ssd1306_t *dev, const u8 *framebuffer, u8 page_mask) {
    if (dev == NULL || framebuffer == NULL || page_mask == 0) {
        if (page_mask == 0) {
            return 0;
        }
        errno = EINVAL;
        return -1;
    }

    for (u8 page = 0; page < OLEDTTY_PAGES; ++page) {
        if ((page_mask & (u8)(1U << page)) == 0) {
            continue;
        }

        const u8 set_page_cmds[] = {
            0x21, 0x00, 0x7F,
            0x22, page, page
        };

        if (ssd1306_send_commands(dev, set_page_cmds, (u16)sizeof(set_page_cmds)) < 0) {
            return -1;
        }

        const u16 offset = (u16)page * OLEDTTY_WIDTH;
        if (i2c_write_packet(&dev->bus, 0x40, framebuffer + offset, OLEDTTY_WIDTH) < 0) {
            return -1;
        }
    }

    return 0;
}

int ssd1306_set_contrast(ssd1306_t *dev, u8 contrast) {
    const u8 cmds[] = { 0x81, contrast };
    return ssd1306_send_commands(dev, cmds, (u16)sizeof(cmds));
}

int ssd1306_clear_display(ssd1306_t *dev) {
    if (dev == NULL) {
        errno = EINVAL;
        return -1;
    }

    u8 zero_frame[OLEDTTY_FB_SIZE];
    memset(zero_frame, 0, sizeof(zero_frame));
    return ssd1306_flush_frame(dev, zero_frame, (u16)sizeof(zero_frame));
}

int ssd1306_init(ssd1306_t *dev, const char *i2c_path, u8 address) {
    if (dev == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(dev, 0, sizeof(*dev));
    dev->width = OLEDTTY_WIDTH;
    dev->height = OLEDTTY_HEIGHT;
    dev->pages = OLEDTTY_PAGES;

    if (i2c_open_bus(&dev->bus, i2c_path, address) < 0) {
        return -1;
    }

    if (ssd1306_send_commands(dev, ssd1306_init_sequence, (u16)sizeof(ssd1306_init_sequence)) < 0) {
        i2c_close_bus(&dev->bus);
        return -1;
    }

    if (ssd1306_set_contrast(dev, OLEDTTY_CONTRAST) < 0) {
        i2c_close_bus(&dev->bus);
        return -1;
    }

    return 0;
}

void ssd1306_shutdown(ssd1306_t *dev) {
    if (dev == NULL) {
        return;
    }

    if (dev->bus.fd >= 0) {
        const u8 off = 0xAE;
        (void)ssd1306_send_commands(dev, &off, 1U);
    }

    i2c_close_bus(&dev->bus);
}
