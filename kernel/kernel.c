#include <stddef.h>
#include <stdint.h>

#include "drivers/vga.h"
#include "sys/idt.h"
#include "sys/isr.h"
#include "sys/pic.h"
#include "sys/pit.h"
#include "drivers/keyboard.h"
#include "cli/shell.h"

static void kputs(const char* s) {
    while (*s) vga_putc(*s++);
}

void kmain(void) {
    // debug: raw VGA breadcrumbs
    volatile char* v=(volatile char*)0xB8000; v[6]='K'; v[7]=0x07; v[8]='1'; v[9]=0x07;
    vga_init();
    vga_clear();
    v[10]='K'; v[11]=0x07; v[12]='2'; v[13]=0x07;

    // Install IDT so exceptions are handled (avoid triple faults)
    isr_install(); v[18]='K'; v[19]=0x07; v[20]='4'; v[21]=0x07;
    kputs("nox_os minimal kernel\n");
    kputs("Booted to protected mode.\n\n");
    v[14]='K'; v[15]=0x07; v[16]='3'; v[17]=0x07;
    // bring up shell first; keep IRQs disabled; we'll poll for input
    shell_init(); v[34]='K'; v[35]=0x07; v[36]='8'; v[37]=0x07;
    keyboard_init(); v[26]='K'; v[27]=0x07; v[28]='6'; v[29]=0x07;
    // force a prompt in case shell_init output was off-screen
    kputs("\n> ");
    // initial controller status once
    {
        uint8_t st0 = keyboard_status();
        const char hex[] = "0123456789ABCDEF";
        vga_putc('['); vga_putc('s'); vga_putc('t'); vga_putc('=');
        vga_putc(hex[(st0>>4)&0xF]); vga_putc(hex[st0&0xF]); vga_putc(']');
    }

    // Idle loop: busy-poll keyboard so input works even if IRQs are not firing
    for(;;) {
        keyboard_poll();
        // heartbeat + status probe (print 0x64 status every ~large interval)
        static unsigned cnt=0; 
        if ((cnt++ & 0x07FFF)==0) {
            vga_putc('.');
            uint8_t st = keyboard_status();
            // print [st=XX]
            vga_putc('['); vga_putc('s'); vga_putc('t'); vga_putc('=');
            const char hex[] = "0123456789ABCDEF";
            vga_putc(hex[(st>>4)&0xF]); vga_putc(hex[st&0xF]);
            vga_putc(']');
        }
    }
}
