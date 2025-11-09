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

static void ps2_write_dev(uint8_t b){
    ps2_wait_input_clear();
    outb(0x60, b);
}

static int ps2_try_read(uint8_t* out){
    for (int i=0;i<100000;i++){
        if (inb(0x64) & 0x01){ *out = inb(0x60); return 1; }
    }
    return 0;
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

static void put_hex_nibble(uint8_t n){
    n &= 0x0F;
    vga_putc(n < 10 ? ('0'+n) : ('A'+(n-10)));
}
static void put_hex8(uint8_t v){
    put_hex_nibble(v>>4); put_hex_nibble(v);
}

void keyboard_init(void){
    isr_register_handler(33, on_key); // IRQ1
    // Safe path: do not program the controller/device during init to avoid pauses.
    // Leave hardware as-is; rely on polling to consume any bytes if they appear.
}

void keyboard_poll(void){
    uint8_t st;
    while ((st = inb(0x64)) & 0x01){
        uint8_t sc = inb(0x60);
        if (sc == 0xFA || sc == 0xAA) continue; // ignore ACK/BAT
        if (sc & 0x80) continue; // release
        char c = 0;
        if (sc < sizeof(scancode_map)) c = scancode_map[sc];
        vga_putc('*'); vga_putc('['); put_hex8(st); vga_putc(':'); put_hex8(sc); vga_putc(']');
        if (c) shell_handle_char(c);
    }
}

uint8_t keyboard_status(void){
    return inb(0x64);
}
