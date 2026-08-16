#include <stddef.h>
#include <stdint.h>

#include "drivers/vga.h"
#include "sys/idt.h"
#include "sys/isr.h"
#include "sys/pic.h"
#include "sys/pit.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"
#include "cli/shell.h"

static void kputs(const char* s) {
    while (*s) vga_putc(*s++);
}

void kmain(void) {
    vga_init();
    vga_clear();

    // Install IDT so exceptions are handled (avoid triple faults)
    isr_install();
    pic_remap(0x20, 0x28);
    kputs("nox_os minimal kernel\n");
    kputs("Booted to protected mode.\n\n");

    shell_init();
    keyboard_init();
    pic_enable_irq(1);
    __asm__ __volatile__("sti");
    serial_init();

    // Idle loop: keyboard input arrives via IRQ1 (on_key in keyboard.c).
    // Serial has no interrupt handler yet, so it's still polled here.
    for (;;) {
        serial_poll();
    }
}
