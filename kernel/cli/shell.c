#include <stdint.h>
#include <stddef.h>
#include "shell.h"
#include "../drivers/vga.h"
#include "../lib/string.h"
#include "../sys/pit.h"
#include "../sys/reboot.h"

#define SHELL_BUFSZ 128
static char line[SHELL_BUFSZ];
static size_t len = 0;

static void putstr(const char* s){ while(*s) vga_putc(*s++); }
static void prompt(void){ putstr("\n> "); }

static int streq(const char* a, const char* b){
    while(*a && *b && *a==*b){ a++; b++; }
    return *a==0 && *b==0;
}

static const char* skipsp(const char* s){ while(*s==' ') s++; return s; }

static void cmd_help(void){
    putstr("Commands:\n");
    putstr(" help\n echo <text>\n meminfo\n ps\n ls\n cat <file>\n reboot\n");
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
}

static void cmd_ps(void){ putstr("no tasks\n"); }
static void cmd_ls(void){ putstr("fs not ready\n"); }
static void cmd_cat(const char* args){ (void)args; putstr("fs not ready\n"); }
static void cmd_reboot(void){ reboot(); }

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
    else if (streq(cmd,"ps")) cmd_ps();
    else if (streq(cmd,"ls")) cmd_ls();
    else if (streq(cmd,"cat")) cmd_cat(args);
    else if (streq(cmd,"reboot")) cmd_reboot();
    else { putstr("unknown command\n"); }
}

void shell_init(void){ len = 0; line[0]=0; putstr("> "); }

void shell_handle_char(char c){
    if (c=='\n'){
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
