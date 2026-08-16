#pragma once
#include <stddef.h>
#include <stdint.h>

// A flat, in-memory filesystem: no directories, nothing persists across
// reboot (there's no real disk driver or on-disk format yet -- everything
// lives on the heap). File content size is capped by the heap allocator's
// own single-frame-per-allocation limit (see mm/heap.h), with a small
// safety margin, so ~4000 bytes max per file for now.

#define FS_MAX_NAME 32
#define FS_MAX_FILE_SIZE 4000u

typedef struct fs_file {
    char name[FS_MAX_NAME];
    uint8_t* data;   // NULL if size == 0
    size_t size;
    struct fs_file* next;
} fs_file_t;

void fs_init(void);

// Creates the file if it doesn't already exist, then overwrites its content
// with [data, len). Returns 0 on success, -1 on error (empty/too-long name,
// len too large, or out of heap memory).
int fs_write(const char* name, const uint8_t* data, size_t len);

// Looks up a file by name. Returns NULL if it doesn't exist.
fs_file_t* fs_find(const char* name);

// Removes a file. Returns 0 on success, -1 if it didn't exist.
int fs_delete(const char* name);

// Returns the first file (iterate via ->next), or NULL if the filesystem
// is empty. Order is most-recently-created first.
fs_file_t* fs_list(void);
