#include "heap.h"
#include "pmm.h"
#include <stdint.h>

#define FRAME_SIZE 4096u

typedef struct block_header {
    size_t size;                // usable payload size, excludes this header
    int free;
    struct block_header* next;  // all blocks (used and free), in creation order
    struct block_header* prev;
} block_header_t;

#define HDR_SIZE ((size_t)sizeof(block_header_t))
#define MIN_SPLIT_PAYLOAD 16u   // don't split off a remainder smaller than this

static block_header_t* heap_head = 0; // first block ever created
static block_header_t* heap_tail = 0; // last block ever created (append point)
static size_t bytes_total = 0;        // total payload bytes ever added (used+free)

void heap_init(void){
    heap_head = 0;
    heap_tail = 0;
    bytes_total = 0;
}

// Pull one more physical frame from the frame allocator and append it to
// the heap as one new free block. Returns 1 on success, 0 if out of frames.
static int heap_grow(void){
    void* frame = pmm_alloc_frame();
    if (!frame) return 0;
    block_header_t* blk = (block_header_t*)frame;
    blk->size = FRAME_SIZE - HDR_SIZE;
    blk->free = 1;
    blk->next = 0;
    blk->prev = heap_tail;
    if (heap_tail) heap_tail->next = blk;
    heap_tail = blk;
    if (!heap_head) heap_head = blk;
    bytes_total += blk->size;
    return 1;
}

void* kmalloc(size_t size){
    if (size == 0) return (void*)0;
    size = (size + 7u) & ~((size_t)7u); // 8-byte alignment

    if (size > FRAME_SIZE - HDR_SIZE) return (void*)0; // see heap.h limitation

    for (;;) {
        for (block_header_t* b = heap_head; b; b = b->next){
            if (b->free && b->size >= size){
                if (b->size >= size + HDR_SIZE + MIN_SPLIT_PAYLOAD){
                    uint8_t* base = (uint8_t*)b;
                    block_header_t* rem = (block_header_t*)(base + HDR_SIZE + size);
                    rem->size = b->size - size - HDR_SIZE;
                    rem->free = 1;
                    rem->next = b->next;
                    rem->prev = b;
                    if (b->next) b->next->prev = rem;
                    b->next = rem;
                    if (heap_tail == b) heap_tail = rem;
                    b->size = size;
                }
                b->free = 0;
                return (void*)((uint8_t*)b + HDR_SIZE);
            }
        }
        if (!heap_grow()) return (void*)0; // out of physical memory
    }
}

void kfree(void* ptr){
    if (!ptr) return;
    block_header_t* b = (block_header_t*)((uint8_t*)ptr - HDR_SIZE);
    b->free = 1;

    // Coalesce with next block only if it's free AND physically adjacent
    // (list order is creation order, not necessarily address order, so this
    // check is what actually makes coalescing safe, not the list links alone)
    if (b->next && b->next->free &&
        (uint8_t*)b + HDR_SIZE + b->size == (uint8_t*)b->next){
        block_header_t* n = b->next;
        b->size += HDR_SIZE + n->size;
        b->next = n->next;
        if (n->next) n->next->prev = b;
        if (heap_tail == n) heap_tail = b;
    }
    // Coalesce with previous block only if it's free AND physically adjacent
    if (b->prev && b->prev->free &&
        (uint8_t*)b->prev + HDR_SIZE + b->prev->size == (uint8_t*)b){
        block_header_t* p = b->prev;
        p->size += HDR_SIZE + b->size;
        p->next = b->next;
        if (b->next) b->next->prev = p;
        if (heap_tail == b) heap_tail = p;
    }
}

size_t kheap_bytes_total(void){ return bytes_total; }

size_t kheap_bytes_free(void){
    size_t f = 0;
    for (block_header_t* b = heap_head; b; b = b->next) if (b->free) f += b->size;
    return f;
}
