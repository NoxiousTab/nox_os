#include <stdint.h>
#include <stddef.h>
#include "shell.h"
#include "../drivers/vga.h"
#include "../lib/string.h"
#include "../sys/pit.h"
#include "../sys/reboot.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../fs/fs.h"
#include "../task/task.h"

#define SHELL_BUFSZ 128
static char line[SHELL_BUFSZ];
static size_t len = 0;

static void putstr(const char* s){ while(*s) vga_putc(*s++); }

static void putdec(uint32_t v){
    char buf[12]; int i=0;
    if (v==0) buf[i++]='0';
    while(v){ buf[i++] = '0' + (v%10); v/=10; }
    while(i--) vga_putc(buf[i]);
}

static void puthex(uint32_t v){
    const char hex[] = "0123456789ABCDEF";
    putstr("0x");
    for (int shift=28; shift>=0; shift-=4) vga_putc(hex[(v>>shift)&0xF]);
}

static int streq(const char* a, const char* b){
    while(*a && *b && *a==*b){ a++; b++; }
    return *a==0 && *b==0;
}

static const char* skipsp(const char* s){ while(*s==' ') s++; return s; }

// Copies the first whitespace-delimited token from s into out (bounded by
// outsz), returns a pointer to whatever follows (after skipping the token
// and any following spaces).
static const char* read_token(const char* s, char* out, size_t outsz){
    s = skipsp(s);
    size_t i = 0;
    while (*s && *s != ' ' && i < outsz-1) { out[i++] = *s++; }
    out[i] = 0;
    return skipsp(s);
}

static void cmd_help(void){
    putstr("Commands:\n");
    putstr(" help\n echo <text>\n meminfo\n palloc\n pfree\n halloc <bytes>\n hfree\n pftest\n spawn <name>\n ps\n ls\n cat <file>\n write <file> <text>\n rm <file>\n reboot\n");
}

static void cmd_echo(const char* args){ putstr(args); vga_putc('\n'); }

static void cmd_meminfo(void){
    putstr("ticks: ");
    uint64_t t = pit_ticks();
    // very minimal decimal print
    char buf[32]; int i=0; if (t==0){ buf[i++]='0'; }
    while(t){ buf[i++] = '0' + (t%10); t/=10; }
    while(i--) vga_putc(buf[i]);
    vga_putc('\n');

    putstr("frames: ");
    putdec((uint32_t)pmm_free_frames());
    putstr(" free / ");
    putdec((uint32_t)pmm_total_frames());
    putstr(" total (4KiB each)\n");

    putstr("heap: ");
    putdec((uint32_t)kheap_bytes_free());
    putstr(" free / ");
    putdec((uint32_t)kheap_bytes_total());
    putstr(" bytes (grows by 4KiB frames)\n");

    putstr("paging: identity-mapped, 0x0-0xFFFFFF (16MiB)\n");
}

static void cmd_ps(void){
    task_t* head = task_list();
    task_t* t = head;
    putstr("ID  STATE       RUNS  NAME\n");
    do {
        putdec(t->id); putstr("   ");
        switch (t->state) {
            case TASK_RUNNING:    putstr("running     "); break;
            case TASK_READY:      putstr("ready       "); break;
            case TASK_TERMINATED: putstr("terminated  "); break;
            default:              putstr("?           "); break;
        }
        putdec(t->run_count); putstr("     ");
        putstr(t->name); putstr("\n");
        t = t->next;
    } while (t != head);
}

static void cmd_ls(void){
    fs_file_t* f = fs_list();
    if (!f) { putstr("(empty)\n"); return; }
    for (; f; f = f->next) {
        putstr(f->name);
        putstr("  ");
        putdec((uint32_t)f->size);
        putstr(" bytes\n");
    }
}

static void cmd_cat(const char* args){
    char fname[FS_MAX_NAME];
    read_token(args, fname, sizeof(fname));
    if (fname[0] == 0) { putstr("usage: cat <file>\n"); return; }
    fs_file_t* f = fs_find(fname);
    if (!f) { putstr("cat: no such file: "); putstr(fname); putstr("\n"); return; }
    for (size_t i=0; i<f->size; i++) vga_putc((char)f->data[i]);
    if (f->size == 0 || f->data[f->size-1] != '\n') vga_putc('\n');
}

static void cmd_write(const char* args){
    char fname[FS_MAX_NAME];
    const char* rest = read_token(args, fname, sizeof(fname));
    if (fname[0] == 0) { putstr("usage: write <file> <text>\n"); return; }
    size_t tlen = strlen(rest);
    if (fs_write(fname, (const uint8_t*)rest, tlen) != 0) {
        putstr("write failed (bad name, content too large, or out of memory)\n");
        return;
    }
    putstr("wrote "); putdec((uint32_t)tlen); putstr(" bytes to "); putstr(fname); putstr("\n");
}

static void cmd_rm(const char* args){
    char fname[FS_MAX_NAME];
    read_token(args, fname, sizeof(fname));
    if (fname[0] == 0) { putstr("usage: rm <file>\n"); return; }
    if (fs_delete(fname) != 0) { putstr("rm: no such file: "); putstr(fname); putstr("\n"); return; }
    putstr("removed "); putstr(fname); putstr("\n");
}

static void cmd_reboot(void){ reboot(); }

static void cmd_pftest(void){
    putstr("Triggering a deliberate page fault (reading unmapped memory at 0x02000000).\n");
    putstr("This will halt the system via the new page-fault handler -- that's expected;\n");
    putstr("restart QEMU afterward.\n");
    volatile uint32_t* bad = (volatile uint32_t*)0x02000000;
    uint32_t v = *bad; // 32MiB is well past our 16MiB identity-mapped region
    (void)v;
    putstr("(this line should never print)\n");
}

static void* last_alloc = 0;
static void cmd_palloc(void){
    void* f = pmm_alloc_frame();
    if (!f) { putstr("out of frames\n"); return; }
    last_alloc = f;
    putstr("allocated frame at "); puthex((uint32_t)f); putstr("\n");
}
static void cmd_pfree(void){
    if (!last_alloc) { putstr("no frame to free (run palloc first)\n"); return; }
    puthex((uint32_t)last_alloc); putstr(" freed\n");
    pmm_free_frame(last_alloc);
    last_alloc = 0;
}

static void* last_halloc = 0;
static void cmd_halloc(const char* args){
    size_t sz = 0;
    const char* p = skipsp(args);
    while (*p >= '0' && *p <= '9') { sz = sz*10 + (size_t)(*p - '0'); p++; }
    if (sz == 0) { putstr("usage: halloc <bytes>\n"); return; }
    void* ptr = kmalloc(sz);
    if (!ptr) { putstr("kmalloc failed (out of heap memory, or size too large for one frame)\n"); return; }
    last_halloc = ptr;
    putstr("allocated "); putdec((uint32_t)sz); putstr(" bytes at "); puthex((uint32_t)ptr); putstr("\n");
}
static void cmd_hfree(void){
    if (!last_halloc) { putstr("no heap block to free (run halloc first)\n"); return; }
    puthex((uint32_t)last_halloc); putstr(" freed\n");
    kfree(last_halloc);
    last_halloc = 0;
}

// A demo task with no purpose other than to prove cooperative scheduling
// works: it just yields forever. Its run_count (visible via `ps`) climbing
// each time you check is the visible proof that the kernel's idle loop is
// actually handing it turns.
// A demo task with no purpose other than to prove timer-gated cooperative
// scheduling works: it holds the CPU until its quantum expires (checked via
// task_check_preempt()), then yields. Its run_count (visible via `ps`)
// climbing in small, steady, real-time-paced amounts -- rather than
// millions of switches per second -- is the visible proof a genuine ~50ms
// time slice is being enforced, not just "yield as fast as the CPU can
// loop" like before.
static void demo_task(void){
    for (;;) { task_check_preempt(); }
}

static void cmd_spawn(const char* args){
    char name[TASK_NAME_MAX];
    read_token(args, name, sizeof(name));
    if (name[0] == 0) { putstr("usage: spawn <name>\n"); return; }
    task_t* t = task_create(name, demo_task);
    if (!t) { putstr("spawn failed (out of heap memory or physical frames)\n"); return; }
    putstr("spawned task id "); putdec(t->id); putstr(" ("); putstr(name); putstr(")\n");
}

static void execute(const char* cmdline){
    const char* s = skipsp(cmdline);
    // find command token
    const char* p = s;
    while(*p && *p!=' ') p++;
    // copy command
    char cmd[16]; size_t ci=0; const char* it=s; while(it<p && ci<sizeof(cmd)-1){ cmd[ci++]=*it++; } cmd[ci]=0;
    const char* args = skipsp(p);

    if (streq(cmd,"")) { /* empty */ }
    else if (streq(cmd,"help")) cmd_help();
    else if (streq(cmd,"echo")) cmd_echo(args);
    else if (streq(cmd,"meminfo")) cmd_meminfo();
    else if (streq(cmd,"palloc")) cmd_palloc();
    else if (streq(cmd,"pfree")) cmd_pfree();
    else if (streq(cmd,"halloc")) cmd_halloc(args);
    else if (streq(cmd,"hfree")) cmd_hfree();
    else if (streq(cmd,"pftest")) cmd_pftest();
    else if (streq(cmd,"spawn")) cmd_spawn(args);
    else if (streq(cmd,"ps")) cmd_ps();
    else if (streq(cmd,"ls")) cmd_ls();
    else if (streq(cmd,"cat")) cmd_cat(args);
    else if (streq(cmd,"write")) cmd_write(args);
    else if (streq(cmd,"rm")) cmd_rm(args);
    else if (streq(cmd,"reboot")) cmd_reboot();
    else { putstr("unknown command\n"); }
}

void shell_init(void){ len = 0; line[0]=0; putstr("> "); }

void shell_handle_char(char c){
    if (c=='\n'){
        putstr("\n");
        line[len]=0;
        execute(line);
        len=0; line[0]=0;
        putstr("> ");
        return;
    }
    if (c=='\b'){
        if (len>0){ len--; putstr("\b"); }
        return;
    }
    if (len+1 < SHELL_BUFSZ){ line[len++]=c; line[len]=0; vga_putc(c); }
}
