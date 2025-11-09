#pragma once
#include <stdint.h>

void serial_init(void);
void serial_poll(void);
uint8_t serial_status(void);
