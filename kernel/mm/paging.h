#pragma once
#include <stdint.h>

// Sets up an identity-mapped page directory/tables covering the low 16MiB
// (the kernel image at 0x10000, VGA memory at 0xB8000, and the entire
// pmm-managed frame pool from 1MiB-16MiB), registers a page-fault handler
// (vector 14) that prints diagnostics and halts (no fault recovery exists
// yet), and enables paging (CR0.PG). Since it's identity-mapped, every
// address stays numerically the same as before -- this just turns the MMU
// on underneath memory that already works.
//
// KNOWN LIMITATION: identity-mapped only -- there is exactly one address
// space (the kernel's), virtual == physical everywhere. Per-process address
// spaces come later, once there are processes to give them to.
void paging_init(void);
