#ifndef OLEDTTY_I2C_H
#define OLEDTTY_I2C_H

#include "types.h"

typedef struct {
    int fd;
    u8 address;
    char device_path[64];
} i2c_bus_t;

int  i2c_open_bus(i2c_bus_t *bus, const char *device_path, u8 address);
void i2c_close_bus(i2c_bus_t *bus);
int  i2c_write_packet(i2c_bus_t *bus, u8 control_byte, const u8 *payload, u16 payload_len);

#endif
