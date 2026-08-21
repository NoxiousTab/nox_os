#include "task.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"

#define TASK_STACK_SIZE 4096u

static task_t main_task;
static task_t* current_task = 0;
static uint32_t next_id = 0;
static uint32_t total_tasks = 0;

// Saves the current callee-saved registers and stack pointer to
// *old_esp_ptr, then switches the stack to new_esp and restores that
// context. naked: no compiler-generated prologue/epilogue, since this
// function's entire point is manually controlling the stack layout, and a
// compiler-inserted prologue would corrupt the offsets used below.
//
// Verified with a standalone native test harness before being used here:
// real interleaved execution between multiple tasks, correct per-task
// local-variable preservation across switches (proving the full register +
// stack state genuinely round-trips), correct termination handling.
__attribute__((naked)) static void task_switch(uint32_t* old_esp_ptr, uint32_t new_esp) {
    (void)old_esp_ptr; (void)new_esp; // silence unused-parameter warnings; naked body ignores C-level params
    __asm__ __volatile__(
        "push %ebp\n"
        "push %ebx\n"
        "push %esi\n"
        "push %edi\n"
        "mov 20(%esp), %eax\n"   // old_esp_ptr (4 pushes = 16 bytes, +4 return addr = 20)
        "mov %esp, (%eax)\n"     // *old_esp_ptr = current esp
        "mov 24(%esp), %esp\n"   // esp = new_esp (16 + 4 + 4 = 24)
        "pop %edi\n"
        "pop %esi\n"
        "pop %ebx\n"
        "pop %ebp\n"
        "ret\n"
    );
}

// Entry point for a freshly-created task's fabricated initial stack (see
// task_create). EBX holds the task's real entry function, placed there by
// task_create and delivered here via task_switch's register-restore pops.
__attribute__((naked)) static void task_trampoline(void) {
    __asm__ __volatile__(
        "call *%ebx\n"
        "call task_exit\n"
        "1: hlt\n"
        "jmp 1b\n"
    );
}

static void copy_name(char* dst, const char* src){
    uint32_t i = 0;
    while (src[i] && i < TASK_NAME_MAX - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

void task_init(void){
    copy_name(main_task.name, "kernel");
    main_task.id = 0;
    main_task.state = TASK_RUNNING;
    main_task.run_count = 0;
    main_task.next = &main_task;
    current_task = &main_task;
    next_id = 1;
    total_tasks = 1;
}

task_t* task_create(const char* name, void (*entry)(void)){
    task_t* t = (task_t*)kmalloc(sizeof(task_t));
    if (!t) return (task_t*)0;
    void* stack = pmm_alloc_frame();
    if (!stack) { kfree(t); return (task_t*)0; }

    copy_name(t->name, name);
    t->id = next_id++;
    t->state = TASK_READY;
    t->run_count = 0;

    // Fabricate an initial stack that looks like a task_switch already
    // saved it: when first switched to, the pop sequence in task_switch
    // will pull `entry` into ebx and then `ret` into task_trampoline,
    // which immediately calls through ebx.
    uint32_t* sp = (uint32_t*)((uint8_t*)stack + TASK_STACK_SIZE);
    *(--sp) = (uint32_t)task_trampoline; // "return address" for task_switch's ret
    *(--sp) = 0;                          // ebp (unused)
    *(--sp) = (uint32_t)entry;            // ebx -- read by task_trampoline
    *(--sp) = 0;                          // esi (unused)
    *(--sp) = 0;                          // edi (unused)
    t->esp = (uint32_t)sp;

    // Insert into the circular list right after the current task.
    t->next = current_task->next;
    current_task->next = t;

    total_tasks++;
    return t;
}

void task_yield(void){
    task_t* prev = current_task;
    task_t* next = prev->next;
    uint32_t tries = 0;
    while (next->state == TASK_TERMINATED && tries < total_tasks + 1){
        next = next->next;
        tries++;
    }
    if (next == prev || next->state == TASK_TERMINATED) return; // nothing else ready

    if (prev->state == TASK_RUNNING) prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    next->run_count++;
    current_task = next;
    task_switch(&prev->esp, next->esp);
}

void task_exit(void){
    current_task->state = TASK_TERMINATED;
    for (;;) task_yield();
}

#define SCHED_QUANTUM_TICKS 5u // ~50ms at the PIT's 100Hz rate (see sys/pit.c)
static volatile uint32_t ticks_since_switch = 0;
static volatile int need_resched = 0;

void task_notify_tick(void){
    if (++ticks_since_switch >= SCHED_QUANTUM_TICKS) {
        ticks_since_switch = 0;
        need_resched = 1;
    }
}

void task_check_preempt(void){
    if (need_resched) {
        need_resched = 0;
        task_yield();
    }
}

task_t* task_list(void){ return &main_task; }
uint32_t task_count(void){ return total_tasks; }
