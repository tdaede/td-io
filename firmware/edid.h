#ifndef EDID_H_
#define EDID_H_

#include "i2c_fifo.h"
#include "i2c_slave.h"

void edid_init(void);

static void edid_handler(i2c_inst_t *i2c, i2c_slave_event_t event);

#endif // EDID_H_
