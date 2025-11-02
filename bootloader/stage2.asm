; nox_os stage2 loader: enable A20, load kernel to 0x00100000, enter pmode, jump to kernel
; Assembled with: nasm -f bin stage2.asm -o stage2.bin

BITS 16
ORG 0x7E00

%define CODE_SEL 0x08
%define DATA_SEL 0x10
%include "build.inc"

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    ; save boot drive
    mov [boot_drive], dl

    ; debug: announce stage2
    mov si, msg_s2
.print_s2:
    lodsb
    or al, al
    jz .after_s2
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .print_s2
.after_s2:

    ; DL contains boot drive from BIOS/previous stage

    ; Enable A20 via BIOS (INT 15h, AX=2401h). Ignore errors for QEMU/Bochs.
    mov ax, 0x2401
    int 0x15
    ; Fast A20 via port 0x92 (System Control Port A)
    in al, 0x92
    or al, 00000010b
    out 0x92, al

    ; Prepare to load kernel at physical 0x00100000 using INT 13h extensions (LBA)
    mov bx, 0x0000
    mov ax, 0x1000        ; ES:BX = 0x1000:0x0000 -> 0x00100000
    mov es, ax

    ; Total sectors to load and starting LBA
    mov word [total_secs], KERNEL_SECTORS
    ; LBA start for kernel: 1 (MBR) + 4 (stage2 padded) = 5
    mov word [lba_low+0], 5
    mov word [lba_low+2], 0
    mov word [lba_high+0], 0
    mov word [lba_high+2], 0
    mov word [iter_guard], 2048   ; prevent infinite loops
    mov word [prev_rem], 0xFFFF

.load_next:
    mov ax, [total_secs]
    cmp ax, 0
    je .loaded
    ; guard
    dec word [iter_guard]
    jnz .guard_ok
    jmp .disk_err
.guard_ok:
    ; up to 32 sectors per call, but ensure transfer does NOT cross 64KiB boundary
    mov cx, 8
    cmp ax, cx
    jbe .ax_ok1
    mov ax, cx
.ax_ok1:
    ; limit by boundary: max = (0x10000 - BX) >> 9, compute as (0xFFFF - BX + 1)
    mov dx, 0xFFFF
    sub dx, bx
    inc dx
    shr dx, 9
    cmp dx, 0
    jne .have_dx
    mov dx, 1
.have_dx:
    cmp ax, dx
    jbe .use_ax
    mov ax, dx
.use_ax:
    ; fill DAP
    mov byte [dap+0], 0x10       ; size
    mov byte [dap+1], 0x00       ; reserved
    mov word [dap+2], ax         ; sectors to read
    mov word [dap+4], bx         ; offset
    mov ax, es
    mov word [dap+6], ax         ; segment
    ; LBA low dword (two words)
    mov ax, [lba_low+0]
    mov word [dap+8], ax
    mov ax, [lba_low+2]
    mov word [dap+10], ax
    ; LBA high dword (two words)
    mov ax, [lba_high+0]
    mov word [dap+12], ax
    mov ax, [lba_high+2]
    mov word [dap+14], ax

    ; INT 13h extensions read (AH=42h)
    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc .disk_err
    ; use actual sectors transferred from DAP
    mov ax, [dap+2]
    test ax, ax
    jz .disk_err
    mov [sectors_now], ax

    ; debug: kernel loaded
    mov si, msg_ld
.print_ld:
    lodsb
    or al, al
    jz .after_ld
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .print_ld
.after_ld:

    ; advance buffer by sectors_now*512 (<= 0x4000). At most one 64KiB wrap.
    mov cx, [sectors_now]
    mov ax, cx
    shl ax, 9            ; bytes = sectors * 512
    add bx, ax
    jc .inc_es
    jmp .no_wrap
.inc_es:
    mov ax, es
    add ax, 0x1000
    mov es, ax
.no_wrap:

    ; advance LBA by sectors read (cx): 32-bit add using 16-bit halves
    ; lba_low += CX
    mov ax, [lba_low+0]
    add ax, cx
    mov [lba_low+0], ax
    mov ax, [lba_low+2]
    adc ax, 0
    mov [lba_low+2], ax
    ; carry into high dword if ever needed (unlikely here)
    mov ax, [lba_high+0]
    adc ax, 0
    mov [lba_high+0], ax
    mov ax, [lba_high+2]
    adc ax, 0
    mov [lba_high+2], ax

    ; decrement remaining by sectors_now with floor at zero
    mov ax, [total_secs]
    mov [prev_rem], ax
    mov cx, [sectors_now]
    cmp ax, cx
    jbe .finish
    sub ax, cx
    mov [total_secs], ax
    jmp .load_next
.finish:
    mov word [total_secs], 0
    jmp .load_next

.loaded:

    ; setup GDT (make sure DS = 0 in case BIOS changed it)
    cli
    xor ax, ax
    mov ds, ax
    lgdt [gdt_descriptor]

    ; last BIOS print BEFORE switching to protected mode
    mov si, msg_pm
.print_pm:
    lodsb
    or al, al
    jz .enable_pmode
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .print_pm

.enable_pmode:
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    ; Far jump to load CS with CODE_SEL
    jmp CODE_SEL:pmode_entry

.disk_err:
    mov si, diskmsg
.pr:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E
    mov bx, 0x0004
    int 0x10
    jmp .pr
.halt:
    hlt
    jmp .halt

[BITS 32]
pmode_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x00020000   ; simple stack above kernel load address

    ; Debug: write 'PM>' and first dword of kernel at 0xB8000
    mov edi, 0xB8000
    mov ax, 0x0720        ; ' ' with attr
    mov ecx, 80*25
    rep stosw             ; clear screen
    mov edi, 0xB8000
    mov byte [edi], 'P'
    mov byte [edi+1], 0x07
    mov byte [edi+2], 'M'
    mov byte [edi+3], 0x07
    mov byte [edi+4], '>'
    mov byte [edi+5], 0x07
    add edi, 12
    ; read dword at 0x00010000
    mov eax, [dword 0x00010000]
    ; print 8 hex chars
    mov ecx, 8
.hex_loop:
    rol eax, 4
    mov bl, al
    and bl, 0x0F
    mov bh, 0x07
    mov dl, bl
    add dl, '0'
    cmp bl, 9
    jbe .digit
    add dl, 7
.digit:
    mov byte [edi], dl
    mov byte [edi+1], bh
    add edi, 2
    loop .hex_loop

    ; Jump to kernel entry at 0x00010000
    mov eax, 0x00010000
    jmp eax

; -----------------
; GDT
; -----------------
[BITS 16]
ALIGN 8
gdt_start:
    dq 0x0000000000000000 ; null
    dq 0x00CF9A000000FFFF ; code segment 0x08
    dq 0x00CF92000000FFFF ; data segment 0x10

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_end:

; Disk Address Packet for INT 13h extensions
ALIGN 16
dap:
    db 0x10, 0x00       ; size, reserved
    dw 0                ; sectors to read
    dw 0                ; buffer offset
    dw 0                ; buffer segment
    dd 0                ; LBA low dword
    dd 0                ; LBA high dword

total_secs dw 0
lba_low    dd 0
lba_high   dd 0
boot_drive db 0
sectors_now dw 0
iter_guard dw 0
prev_rem dw 0

diskmsg db "Stage2 disk error!",0
msg_s2 db "[S2]",0
msg_ld db "[LD]",0
msg_pm db "[PM]",0

; pad to 4 sectors total size (2048 bytes) if needed by build; otherwise safe to be smaller
TIMES (2048 - ($-$$)) db 0
