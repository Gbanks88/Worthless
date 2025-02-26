; Multiboot header for GRUB compatibility
MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x00000003
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .text
global _start

_start:
    ; Set up stack
    mov esp, stack_top
    
    ; Initialize platform-specific features
    call detect_platform
    
    ; Enter kernel
    call kernel_main
    
    ; Halt if we return
    cli
.halt:
    hlt
    jmp .halt

detect_platform:
    ; Basic platform detection logic
    ; Will be expanded later
    ret

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB stack
stack_top:
