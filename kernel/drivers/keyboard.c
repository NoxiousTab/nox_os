#include <stdint.h>
#include "keyboard.h"
#include "../sys/io.h"
#include "../sys/isr.h"
#include "vga.h"
#include "../cli/shell.h"
#include "../sys/pic.h"

static const char scancode_map[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'', '`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
};

static void ps2_wait_input_clear(void){
    // Wait for input buffer (IBF) clear (bit1 == 0)
    for (int i=0;i<100000;i++) { if ((inb(0x64) & 0x02) == 0) return; }
}

static void ps2_wait_output_full(void){
    // Wait for output buffer (OBF) full (bit0 == 1)
    for (int i=0;i<100000;i++) { if (inb(0x64) & 0x01) return; }
}

static void ps2_flush(void){
    // Drain any pending data
    while (inb(0x64) & 0x01) { (void)inb(0x60); }
}

static void on_key(struct regs* r){
    (void)r;
    uint8_t sc = inb(0x60);
    if (sc & 0x80) return; // key release ignored
    char c = 0;
    if (sc < sizeof(scancode_map)) c = scancode_map[sc];
    if (c) shell_handle_char(c);
    pic_send_eoi(1);
}

void keyboard_init(void){
    isr_register_handler(33, on_key); // IRQ1
    // Robust 8042 init
    ps2_flush();
    // Disable both ports
    ps2_wait_input_clear(); outb(0x64, 0xAD);
    ps2_wait_input_clear(); outb(0x64, 0xA7);
    // Read command byte
    ps2_wait_input_clear(); outb(0x64, 0x20);
    ps2_wait_output_full();
    uint8_t cmd = inb(0x60);
    // Enable IRQ1 and translation
    cmd |= 0x01;   // IRQ1 enable
    cmd |= 0x40;   // translation
    // Write command byte
    ps2_wait_input_clear(); outb(0x64, 0x60);
    ps2_wait_input_clear(); outb(0x60, cmd);
    // Enable first port (keyboard)
    ps2_wait_input_clear(); outb(0x64, 0xAE);
    // Enable device scanning
    ps2_flush();
    ps2_wait_input_clear(); outb(0x60, 0xF4);
    ps2_wait_output_full(); (void)inb(0x60); // ACK 0xFA
}

void keyboard_poll(void){
    if (inb(0x64) & 0x01){
        uint8_t sc = inb(0x60);
        if (sc & 0x80) return;
        char c = 0;
        if (sc < sizeof(scancode_map)) c = scancode_map[sc];
        vga_putc('*');
        if (c) shell_handle_char(c);
    }
}
