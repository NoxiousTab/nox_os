; 32-bit ISR/IRQ stubs
BITS 32

%macro PUSH_ALL 0
    push ds
    push es
    push fs
    push gs
    pushad
%endmacro
%macro POP_ALL 0
    popad
    pop gs
    pop fs
    pop es
    pop ds
%endmacro

extern isr_handler
extern irq_handler

global isr0
isr0:
    push dword 0
    push dword 0
    jmp isr_common_stub

global isr1
isr1:
    push dword 0
    push dword 1
    jmp isr_common_stub

global isr2
isr2:
    push dword 0
    push dword 2
    jmp isr_common_stub

global isr3
isr3:
    push dword 0
    push dword 3
    jmp isr_common_stub

global isr4
isr4:
    push dword 0
    push dword 4
    jmp isr_common_stub

global isr5
isr5:
    push dword 0
    push dword 5
    jmp isr_common_stub

global isr6
isr6:
    push dword 0
    push dword 6
    jmp isr_common_stub

global isr7
isr7:
    push dword 0
    push dword 7
    jmp isr_common_stub

global isr8
isr8:
    push dword 8
    push dword 8
    jmp isr_common_stub

global isr9
isr9:
    push dword 0
    push dword 9
    jmp isr_common_stub

global isr10
isr10:
    push dword 10
    push dword 10
    jmp isr_common_stub

global isr11
isr11:
    push dword 11
    push dword 11
    jmp isr_common_stub

global isr12
isr12:
    push dword 12
    push dword 12
    jmp isr_common_stub

global isr13
isr13:
    push dword 13
    push dword 13
    jmp isr_common_stub

global isr14
isr14:
    push dword 14
    push dword 14
    jmp isr_common_stub

global isr15
isr15:
    push dword 0
    push dword 15
    jmp isr_common_stub

global isr16
isr16:
    push dword 0
    push dword 16
    jmp isr_common_stub

global isr17
isr17:
    push dword 0
    push dword 17
    jmp isr_common_stub

global isr18
isr18:
    push dword 0
    push dword 18
    jmp isr_common_stub

global isr19
isr19:
    push dword 0
    push dword 19
    jmp isr_common_stub

global isr20
isr20:
    push dword 0
    push dword 20
    jmp isr_common_stub

global isr21
isr21:
    push dword 0
    push dword 21
    jmp isr_common_stub

global isr22
isr22:
    push dword 0
    push dword 22
    jmp isr_common_stub

global isr23
isr23:
    push dword 0
    push dword 23
    jmp isr_common_stub

global isr24
isr24:
    push dword 0
    push dword 24
    jmp isr_common_stub

global isr25
isr25:
    push dword 0
    push dword 25
    jmp isr_common_stub

global isr26
isr26:
    push dword 0
    push dword 26
    jmp isr_common_stub

global isr27
isr27:
    push dword 0
    push dword 27
    jmp isr_common_stub

global isr28
isr28:
    push dword 0
    push dword 28
    jmp isr_common_stub

global isr29
isr29:
    push dword 0
    push dword 29
    jmp isr_common_stub

global isr30
isr30:
    push dword 0
    push dword 30
    jmp isr_common_stub

global isr31
isr31:
    push dword 0
    push dword 31
    jmp isr_common_stub

isr_common_stub:
    PUSH_ALL
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call isr_handler
    add esp, 4
    POP_ALL
    add esp, 8
    iretd

; IRQs
%assign i 0
%rep 16
    global irq%+i
irq%+i:
    push dword 0
    push dword (32+i)
    jmp irq_common_stub
%assign i i+1
%endrep

irq_common_stub:
    PUSH_ALL
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call irq_handler
    add esp, 4
    POP_ALL
    add esp, 8
    iretd
