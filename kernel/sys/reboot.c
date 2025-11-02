#include <stdint.h>
#include "io.h"

void reboot(void){
    // Try keyboard controller pulse reset
    // Wait until input buffer empty
    while (inb(0x64) & 0x02) { }
    outb(0x64, 0xFE);
    // Fallback: triple fault by loading empty IDT and triggering int
    __asm__ __volatile__(
        "cli\n\t"
        "xor %%eax, %%eax\n\t"
        "lidt (%%eax)\n\t"
        "int $0x03\n\t"
        : : : "eax");
    for(;;) { __asm__ __volatile__("hlt"); }
}
