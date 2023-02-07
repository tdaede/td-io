#include "edid.h"
#include "pico/stdlib.h"
#include "i2c_slave.h"
#include "i2c_fifo.h"

#define PIN_SDA 4
#define PIN_SCL 5

static const uint8_t edid_value[128] =
    "\x00\xff\xff\xff\xff\xff\xff\x00\x31\xd8\x00\x00\x00\x00\x00\x00"
    "\x05\x21\x01\x03\x6d\x10\x0c\x78\xea\x5e\xc0\xa4\x59\x4a\x98\x25"
    "\x20\x50\x54\x00\x00\x00\x31\x40\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x8c\x0a\x80\xda\x20\xe0\x2d\x10\x18\x60"
    "\xa4\x00\xa6\x7d\x00\x00\x00\x18\x00\x00\x00\xff\x00\x74\x64\x2d"
    "\x69\x6f\x20\x20\x20\x0a\x20\x20\x20\x20\x00\x00\x00\xfd\x00\x3b"
    "\x3d\x1e\x20\x03\x00\x0a\x20\x20\x20\x20\x20\x20\x00\x00\x00\xfc"
    "\x00\x74\x64\x2d\x69\x6f\x0a\x20\x20\x20\x20\x20\x20\x20\x00\xf6";

static int16_t edid_address = 0;

static void edid_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
        case I2C_SLAVE_RECEIVE:
            edid_address = i2c_read_byte(i2c) & 0x7F;
            break;
        case I2C_SLAVE_REQUEST:
            i2c_write_byte(i2c, edid_value[edid_address]);
            edid_address++;
            if (edid_address > 127) edid_address = 0;
            break;
        default:
            break;
    }
}

void edid_init(void) {
    gpio_init(PIN_SDA);
    gpio_init(PIN_SCL);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);

    i2c_init(i2c0, 100000);
    i2c_slave_init(i2c0, 0x50, &edid_handler);
}
