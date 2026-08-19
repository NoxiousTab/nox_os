#include "paging.h"
#include "../sys/isr.h"
#include "../drivers/vga.h"

#define PAGE_PRESENT 0x1u
#define PAGE_RW      0x2u
#define PAGE_SIZE_4K 4096u

// Identity-map the low 16MiB: real-mode/BIOS area, our kernel image (loaded
// at 0x10000), VGA memory (0xB8000), and the entire pmm-managed frame pool
// (1MiB-16MiB). 4 page tables x 1024 entries x 4KiB = 16MiB, matching pmm's
// own pool size exactly (see mm/pmm.c).
#define IDENTITY_MAP_MB 16u
#define NUM_PAGE_TABLES (IDENTITY_MAP_MB / 4u)

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[NUM_PAGE_TABLES][1024] __attribute__((aligned(4096)));

static void kputs(const char* s){ while (*s) vga_putc(*s++); }

static void kputhex(uint32_t v){
    const char hex[] = "0123456789ABCDEF";
    kputs("0x");
    for (int shift = 28; shift >= 0; shift -= 4) vga_putc(hex[(v >> shift) & 0xF]);
}

// We don't have fault recovery (demand paging, copy-on-write, swapping,
// none of that exists yet), so an unhandled page fault means something is
// genuinely wrong. Print diagnostics from CR2 (faulting address) and the
// CPU-provided error code, then halt cleanly instead of letting the default
// handler's silent "print [EXC] and retry the faulting instruction forever"
// behavior spin in place.
static void page_fault_handler(struct regs* r){
    uint32_t fault_addr;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(fault_addr));

    kputs("\n[PAGE FAULT] addr=");
    kputhex(fault_addr);
    kputs(" err=");
    kputhex(r->err_code);
    kputs(" (");
    kputs((r->err_code & 0x1) ? "present" : "not-present");
    kputs(", ");
    kputs((r->err_code & 0x2) ? "write" : "read");
    kputs(", ");
    kputs((r->err_code & 0x4) ? "user" : "supervisor");
    kputs(")\nSystem halted.\n");

    for (;;) { __asm__ __volatile__("cli\nhlt"); }
}

void paging_init(void){
    for (uint32_t t = 0; t < NUM_PAGE_TABLES; t++){
        for (uint32_t i = 0; i < 1024; i++){
            uint32_t phys = (t * 1024u + i) * PAGE_SIZE_4K;
            page_tables[t][i] = phys | PAGE_RW | PAGE_PRESENT;
        }
        page_directory[t] = ((uint32_t)&page_tables[t][0]) | PAGE_RW | PAGE_PRESENT;
    }
    for (uint32_t d = NUM_PAGE_TABLES; d < 1024; d++){
        page_directory[d] = 0; // not present
    }

    isr_register_handler(14, page_fault_handler);

    __asm__ __volatile__(
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        :: "r"(page_directory)
        : "eax"
    );
}
