#pragma once
#include <stdint.h>

// Cooperative kernel-thread tasks: all tasks share the kernel's single
// address space (no per-process page tables yet -- see mm/paging.h's own
// documented limitation), and switching only happens at explicit
// task_yield() calls -- no preemption yet. The PIT interrupt exists and is
// ticking (see sys/pit.h) but isn't wired into the scheduler; that's a
// natural follow-up once this cooperative base is solid. The kernel's own
// idle loop calls task_yield() so spawned tasks actually get a turn.
//
// The context-switch mechanism (task_switch/task_trampoline in task.c) was
// verified with a standalone native test harness -- real interleaved
// execution, correct per-task local-variable preservation across switches,
// correct termination handling -- before being wired into the kernel.
//
// KNOWN LIMITATIONS:
// - Cooperative only, not preemptive -- a task that never calls
//   task_yield() (directly or via something it calls) will starve everyone
//   else, including the shell.
// - Each task's stack is exactly one 4KiB physical frame (allocated via
//   pmm, not kmalloc, since kmalloc's own per-allocation limit is smaller
//   than one full frame). Keep task functions simple.
// - Terminated tasks are marked TERMINATED and skipped by the scheduler,
//   but their stack and control block are never freed/reclaimed yet.
// - Shell commands run inside the keyboard interrupt handler, which can
//   fire and preempt whichever task happens to be executing at that exact
//   instant (true hardware preemption, separate from our cooperative task
//   switching). A command like `spawn` that touches the task list uses
//   whatever `current_task` is at that moment -- not necessarily the
//   `kernel` task -- which affects where the new task lands in the ring
//   and can make scheduling look asymmetric between tasks (observed: one
//   spawned task's run count consistently tracking the kernel task's,
//   another lagging behind). Not corruption, just an emergent effect of
//   async input timing on ring insertion order; worth knowing about before
//   building anything that assumes "fair" round-robin scheduling.

#define TASK_NAME_MAX 32

typedef enum { TASK_READY, TASK_RUNNING, TASK_TERMINATED } task_state_t;

typedef struct task {
    uint32_t id;
    char name[TASK_NAME_MAX];
    task_state_t state;
    uint32_t esp;
    uint32_t run_count;
    struct task* next; // circular list
} task_t;

// Sets up the "main" task representing the kernel's own currently-running
// context (the boot stack). Call once, early, before task_create().
void task_init(void);

// Creates a new task running entry() on its own stack, in the READY state.
// Returns NULL on failure (out of heap memory or physical frames).
task_t* task_create(const char* name, void (*entry)(void));

// Cooperatively switches to the next READY task in the circular list, if
// any. Returns immediately (no-op) if there's nothing else ready to run.
void task_yield(void);

// Marks the current task TERMINATED and yields forever. Called
// automatically if a task's entry function returns; can also be called
// explicitly by a task to end itself early.
void task_exit(void);

// Returns the "main" task (a fixed entry point into the circular list, for
// iterating via ->next until you loop back around to it) -- used by `ps`.
task_t* task_list(void);

uint32_t task_count(void);
