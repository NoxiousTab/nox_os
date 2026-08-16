#pragma once
#include <stdint.h>
#include <stddef.h>

// Physical memory manager: a bitmap-based allocator over fixed-size 4KiB
// frames. This is deliberately the smallest useful step -- no paging, no
// virtual memory, just "is this physical frame in use or not". Paging can
// be built on top of this later.
//
// KNOWN LIMITATION: there is no real memory detection (no E820 map from the
// bootloader) yet, so the pool size below is a conservative hardcoded value
// rather than the machine's actual RAM size. See the comment above PMM_POOL_SIZE
// in pmm.c. Safe on any machine with at least 16MiB of RAM (which is
// effectively all of them, including QEMU's 128MiB default), but not a
// substitute for real memory detection.

void pmm_init(void);

// Returns the physical address of a free 4KiB-aligned frame, and marks it
// used. Returns NULL if no frames are free.
void* pmm_alloc_frame(void);

// Marks a previously-allocated frame as free again. Ignores addresses
// outside the managed pool or not aligned to a frame boundary.
void pmm_free_frame(void* frame);

size_t pmm_total_frames(void);
size_t pmm_free_frames(void);
