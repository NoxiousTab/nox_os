#include <stdint.h>
#include "vga.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
static uint16_t* const VGA_MEM = (uint16_t*)0xB8000;

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t vga_color = 0x07; // light grey on black

static inline uint16_t vga_entry(char c, uint8_t color) {
    return ((uint16_t)color << 8) | (uint8_t)c;
}

void vga_init(void) {
    cursor_x = 0; cursor_y = 0; vga_color = 0x07;
}

void vga_clear(void) {
    for (int y=0; y<VGA_HEIGHT; ++y) {
        for (int x=0; x<VGA_WIDTH; ++x) {
            VGA_MEM[y*VGA_WIDTH + x] = vga_entry(' ', vga_color);
        }
    }
    cursor_x = 0; cursor_y = 0;
}

static void vga_newline(void) {
    cursor_x = 0;
    if (++cursor_y >= VGA_HEIGHT) {
        // scroll up
        for (int y=1; y<VGA_HEIGHT; ++y) {
            for (int x=0; x<VGA_WIDTH; ++x) {
                VGA_MEM[(y-1)*VGA_WIDTH + x] = VGA_MEM[y*VGA_WIDTH + x];
            }
        }
        for (int x=0; x<VGA_WIDTH; ++x) VGA_MEM[(VGA_HEIGHT-1)*VGA_WIDTH + x] = vga_entry(' ', vga_color);
        cursor_y = VGA_HEIGHT-1;
    }
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_newline();
        return;
    }
    VGA_MEM[cursor_y*VGA_WIDTH + cursor_x] = vga_entry(c, vga_color);
    if (++cursor_x >= VGA_WIDTH) vga_newline();
}
