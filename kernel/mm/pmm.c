#include "pmm.h"

#define FRAME_SIZE 4096u

// Manage physical memory starting at 1MiB. Everything below 1MiB (real-mode
// IVT/BIOS data area, our own kernel image which loads at 0x10000, and the
// stack) is left alone entirely, so this allocator never needs to reserve
// or special-case any frame within its own pool.
#define POOL_BASE 0x00100000u

// TODO: this is a hardcoded conservative guess, not real memory detection.
// We don't yet parse a BIOS E820 memory map (a bootloader-side addition),
// so we can't know the machine's actual RAM size. 15MiB above the 1MiB mark
// (16MiB pool end) is safe on essentially any x86 machine or emulator
// (QEMU's default is 128MiB), but a machine with less RAM than this would
// silently hand out frames that don't physically exist. Revisit once
// stage2 grows E820 support.
#define POOL_SIZE 0x00F00000u

#define FRAME_COUNT (POOL_SIZE / FRAME_SIZE)
#define BITMAP_WORDS ((FRAME_COUNT + 31u) / 32u)

// 0 = free, 1 = used
static uint32_t bitmap[BITMAP_WORDS];
static size_t free_count;

static inline void set_bit(size_t i)   { bitmap[i / 32u] |= (1u << (i % 32u)); }
static inline void clear_bit(size_t i) { bitmap[i / 32u] &= ~(1u << (i % 32u)); }
static inline int  test_bit(size_t i)  { return (bitmap[i / 32u] >> (i % 32u)) & 1u; }

void pmm_init(void) {
    for (size_t i = 0; i < BITMAP_WORDS; i++) bitmap[i] = 0;
    free_count = FRAME_COUNT;
}

void* pmm_alloc_frame(void) {
    for (size_t i = 0; i < FRAME_COUNT; i++) {
        if (!test_bit(i)) {
            set_bit(i);
            free_count--;
            return (void*)(POOL_BASE + i * FRAME_SIZE);
        }
    }
    return (void*)0;
}

void pmm_free_frame(void* frame) {
    uint32_t addr = (uint32_t)frame;
    if (addr < POOL_BASE || addr >= POOL_BASE + POOL_SIZE) return;
    if ((addr - POOL_BASE) % FRAME_SIZE != 0) return;
    size_t i = (addr - POOL_BASE) / FRAME_SIZE;
    if (test_bit(i)) {
        clear_bit(i);
        free_count++;
    }
}

size_t pmm_total_frames(void) { return FRAME_COUNT; }
size_t pmm_free_frames(void)  { return free_count; }
