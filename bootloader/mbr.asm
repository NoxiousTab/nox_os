; nox_os MBR (stage1) - loads stage2 from LBA 1 into 0x7E00 and jumps
; Assembled with: nasm -f bin mbr.asm -o mbr.bin

BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Print tiny banner using BIOS teletype
    mov si, bootmsg
.print:
    lodsb
    or al, al
    jz .load_stage2
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .print

.load_stage2:
    ; Load 4 sectors from LBA 1 to 0000:7E00 using INT 13h extensions
    xor ax, ax
    mov es, ax
    mov bx, 0x7E00
    ; prepare DAP
    mov byte [dap+0], 0x10
    mov byte [dap+1], 0x00
    mov word [dap+2], 4          ; sectors
    mov word [dap+4], bx         ; offset
    mov word [dap+6], es         ; segment
    mov word [dap+8], 1          ; LBA low word
    mov word [dap+10], 0
    mov word [dap+12], 0
    mov word [dap+14], 0
    mov si, dap
    mov ah, 0x42
    int 0x13
    jc disk_error

    jmp 0x0000:0x7E00

disk_error:
    mov si, errmsg
.err:
    lodsb
    or al, al
    jz .halt
    mov ah, 0x0E
    mov bx, 0x0004
    int 0x10
    jmp .err
.halt:
    hlt
    jmp .halt

bootmsg db "nox_os boot...",0
errmsg  db "Disk error!",0
BOOT_DRIVE db 0

ALIGN 16
dap:
    db 0x10,0x00
    dw 0
    dw 0
    dw 0
    dd 0
    dd 0

TIMES 510-($-$$) db 0
DW 0xAA55
