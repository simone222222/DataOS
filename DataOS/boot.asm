global start
extern kernel_main

section .text
bits 32
start:
    ; Set up the stack
    mov esp, stack_top

    ; Call the kernel
    call kernel_main

    ; If the kernel returns, just hang
    cli
.hang:
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB
stack_top: