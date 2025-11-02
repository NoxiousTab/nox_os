#pragma once
#include <stdint.h>
void pic_remap(int offset1, int offset2);
void pic_send_eoi(uint8_t irq_vector);
void pic_enable_irq(uint8_t irq_line);
void pic_disable_irq(uint8_t irq_line);
