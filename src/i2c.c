#include "i2c.h"
#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Static packet buffer: control byte + max payload (full framebuffer).
 * Safe because oledtty is single-threaded — no concurrent calls. */
#define I2C_PACKET_MAX (OLEDTTY_FB_SIZE + 1U)
static u8 g_packet[I2C_PACKET_MAX];

int i2c_open_bus(i2c_bus_t *bus, const char *device_path, u8 address) {
    if (bus == NULL || device_path == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(bus, 0, sizeof(*bus));
    bus->address = address;
    snprintf(bus->device_path, sizeof(bus->device_path), "%s", device_path);

    bus->fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (bus->fd < 0) {
        return -1;
    }

    if (ioctl(bus->fd, I2C_SLAVE, address) < 0) {
        close(bus->fd);
        bus->fd = -1;
        return -1;
    }

    return 0;
}

void i2c_close_bus(i2c_bus_t *bus) {
    if (bus == NULL) {
        return;
    }

    if (bus->fd >= 0) {
        close(bus->fd);
        bus->fd = -1;
    }
}

int i2c_write_packet(i2c_bus_t *bus, u8 control_byte, const u8 *payload, u16 payload_len) {
    if (bus == NULL || bus->fd < 0 || payload == NULL) {
        errno = EINVAL;
        return -1;
    }

    const size_t packet_len = (size_t)payload_len + 1U;
    if (packet_len > I2C_PACKET_MAX) {
        errno = EMSGSIZE;
        return -1;
    }

    g_packet[0] = control_byte;
    memcpy(&g_packet[1], payload, payload_len);

    struct i2c_msg msg;
    memset(&msg, 0, sizeof(msg));
    msg.addr = bus->address;
    msg.flags = 0;
    msg.len = (unsigned short)packet_len;
    msg.buf = g_packet;

    struct i2c_rdwr_ioctl_data ioctl_data;
    memset(&ioctl_data, 0, sizeof(ioctl_data));
    ioctl_data.msgs = &msg;
    ioctl_data.nmsgs = 1;

    /* Retry on transient I2C bus errors (EIO, EREMOTEIO=121).
     * Common at 400 kHz+ with long wires or during SSD1306 busy periods.
     * Backoff: ~1ms, ~2ms, ~4ms — total worst-case ~7ms before giving up. */
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (ioctl(bus->fd, I2C_RDWR, &ioctl_data) >= 0) {
            return 0;
        }
        if (errno != EIO && errno != 121 /* EREMOTEIO */) {
            return -1; /* non-transient: EINVAL, ENXIO, etc. */
        }
        usleep((useconds_t)(1000U << attempt));
    }
    return -1;
}
