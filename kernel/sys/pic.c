#include <stdint.h>
#include "io.h"

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)
#define PIC_EOI 0x20

void pic_remap(int offset1, int offset2){
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    outb(PIC1_DATA, (uint8_t)offset1);
    io_wait();
    outb(PIC2_DATA, (uint8_t)offset2);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_send_eoi(uint8_t irq_vector){
    if (irq_vector >= 40) outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_enable_irq(uint8_t irq_line){
    uint16_t port; uint8_t value;
    if (irq_line < 8){ port = PIC1_DATA; }
    else { port = PIC2_DATA; irq_line -= 8; }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);
}

void pic_disable_irq(uint8_t irq_line){
    uint16_t port; uint8_t value;
    if (irq_line < 8){ port = PIC1_DATA; }
    else { port = PIC2_DATA; irq_line -= 8; }
    value = inb(port) | (1 << irq_line);
    outb(port, value);
}
