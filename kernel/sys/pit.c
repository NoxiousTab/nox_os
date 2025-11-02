#include <stdint.h>
#include "io.h"
#include "pit.h"
#include "isr.h"

#define PIT_CH0 0x40
#define PIT_CMD 0x43

static volatile uint64_t ticks = 0;

static void pit_isr(struct regs* r){ (void)r; ticks++; }

uint64_t pit_ticks(void){ return ticks; }

void pit_init(uint32_t freq){
    uint32_t divisor = 1193182 / (freq ? freq : 100);
    outb(PIT_CMD, 0x36); // channel 0, lo+hi, mode 3, binary
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
    isr_register_handler(32, pit_isr); // IRQ0 vector after PIC remap
}
