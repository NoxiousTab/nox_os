#include <stdint.h>
#include "serial.h"
#include "vga.h"
#include "../sys/io.h"
#include "../cli/shell.h"

static inline void com_out(uint16_t port, uint8_t val){ outb(port, val); }
static inline uint8_t com_in(uint16_t port){ return inb(port); }
static inline uint8_t lsr(uint16_t base){ return com_in(base + 5); }
static void serial_putc(char c){
    uint16_t base = 0x3F8;
    while ((lsr(base) & 0x20) == 0) { }
    com_out(base + 0, (uint8_t)c);
}
static void serial_write(const char* s){ while(*s) serial_putc(*s++); }

void serial_init(void){
    uint16_t base = 0x3F8;
    com_out(base + 1, 0x00);
    com_out(base + 3, 0x80);
    com_out(base + 0, 0x01);
    com_out(base + 1, 0x00);
    com_out(base + 3, 0x03);
    com_out(base + 2, 0xC7);
    com_out(base + 4, 0x0B);
    serial_write("[serial=ready]\r\n> ");
}

void serial_poll(void){
    uint16_t base = 0x3F8;
    while (lsr(base) & 0x01){
        uint8_t b = com_in(base + 0);
        char c = (char)b;
        if (c == '\r') c = '\n';
        // visible trace on VGA
        extern void vga_putc(char);
        extern void vga_put_hex(uint8_t);
        vga_putc('#'); vga_putc('[');
        const char hex[] = "0123456789ABCDEF";
        vga_putc(hex[(b>>4)&0xF]); vga_putc(hex[b&0xF]); vga_putc(']');
        if (c) shell_handle_char(c);
    }
}
