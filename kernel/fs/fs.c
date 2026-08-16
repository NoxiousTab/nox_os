#include "fs.h"
#include "../mm/heap.h"
#include "../lib/string.h"
#include "../lib/mem.h"

static fs_file_t* files_head = 0;

static int streq(const char* a, const char* b){
    while (*a && *b && *a==*b) { a++; b++; }
    return *a==0 && *b==0;
}

void fs_init(void){
    files_head = 0;
}

fs_file_t* fs_find(const char* name){
    for (fs_file_t* f = files_head; f; f = f->next)
        if (streq(f->name, name)) return f;
    return 0;
}

fs_file_t* fs_list(void){ return files_head; }

int fs_write(const char* name, const uint8_t* data, size_t len){
    size_t namelen = strlen(name);
    if (namelen == 0 || namelen >= FS_MAX_NAME) return -1;
    if (len > FS_MAX_FILE_SIZE) return -1;

    fs_file_t* f = fs_find(name);
    if (!f) {
        f = (fs_file_t*)kmalloc(sizeof(fs_file_t));
        if (!f) return -1;
        size_t i = 0;
        while (name[i] && i < FS_MAX_NAME - 1) { f->name[i] = name[i]; i++; }
        f->name[i] = 0;
        f->data = 0;
        f->size = 0;
        f->next = files_head;
        files_head = f;
    }

    uint8_t* newdata = 0;
    if (len > 0) {
        newdata = (uint8_t*)kmalloc(len);
        if (!newdata) return -1; // note: old content is left untouched on failure
        memcpy(newdata, data, len);
    }
    if (f->data) kfree(f->data);
    f->data = newdata;
    f->size = len;
    return 0;
}

int fs_delete(const char* name){
    fs_file_t** pp = &files_head;
    while (*pp) {
        if (streq((*pp)->name, name)) {
            fs_file_t* victim = *pp;
            *pp = victim->next;
            if (victim->data) kfree(victim->data);
            kfree(victim);
            return 0;
        }
        pp = &(*pp)->next;
    }
    return -1;
}
