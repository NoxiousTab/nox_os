; kernel 32-bit entry stub
BITS 32
SECTION .text.start
GLOBAL _start
EXTERN kmain
EXTERN __bss_start
EXTERN __bss_end
_start:
    ; Debug: write 'K>' at top-left
    mov edi, 0xB8000
    mov byte [edi], 'K'
    mov byte [edi+1], 0x07
    mov byte [edi+2], '>'
    mov byte [edi+3], 0x07

    ; Zero BSS
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb
    call kmain
.hang:
    hlt
    jmp .hang
