#pragma once
#include <stddef.h>

// A simple explicit free-list heap built on top of the physical frame
// allocator (pmm.h). Grows one 4KiB frame at a time as needed; uses
// first-fit allocation with block splitting, and coalesces adjacent free
// blocks on kfree(). No paging involved -- since paging isn't set up yet,
// physical frame addresses are used directly as heap memory.
//
// KNOWN LIMITATION: a single kmalloc() request must fit within one frame
// (4096 bytes minus a small header), since the heap currently only grows by
// one frame at a time and doesn't support allocations spanning multiple
// frames. Fine for small kernel data structures; revisit if something needs
// a genuinely large single allocation.

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

size_t kheap_bytes_free(void);
size_t kheap_bytes_total(void);
