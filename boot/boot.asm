; tiny-os boot code
; This file sets up the Multiboot2 header and transitions to 64-bit long mode

bits 32

; Multiboot2 header constants
MULTIBOOT2_MAGIC equ 0xe85250d6
MULTIBOOT2_ARCH equ 0  ; i386
HEADER_LENGTH equ (multiboot_header_end - multiboot_header)
CHECKSUM equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + HEADER_LENGTH)

; Page table flags
PAGE_PRESENT equ 1 << 0
PAGE_WRITE equ 1 << 1
PAGE_HUGE equ 1 << 7

section .multiboot
align 8
multiboot_header:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd HEADER_LENGTH
    dd CHECKSUM

    ; Information request tag
    align 8
    dw 1  ; type = information request
    dw 0  ; flags
    dd 16 ; size
    dd 6  ; memory map tag

    ; End tag
    align 8
    dw 0  ; type
    dw 0  ; flags
    dd 8  ; size
multiboot_header_end:

section .bss
align 4096
; Page tables (will be identity mapped initially)
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096

; Kernel stack (16KB)
stack_bottom:
    resb 16384
stack_top:

section .boot
global _start
_start:
    ; Save Multiboot2 magic and info pointer
    mov edi, eax  ; Multiboot2 magic number
    mov esi, ebx  ; Multiboot2 information structure

    ; Set up stack
    mov esp, stack_top

    ; Check if we're loaded by a Multiboot2 bootloader
    cmp eax, 0x36d76289
    jne .no_multiboot

    ; Check for CPUID support
    call check_cpuid
    test eax, eax
    jz .no_cpuid

    ; Check for long mode support
    call check_long_mode
    test eax, eax
    jz .no_long_mode

    ; Set up page tables
    call setup_page_tables

    ; Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Load P4 table into CR3
    mov eax, p4_table
    mov cr3, eax

    ; Enable long mode in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8  ; Set LME (Long Mode Enable)
    wrmsr

    ; Enable paging and protected mode
    mov eax, cr0
    or eax, 1 << 31  ; Set PG (paging)
    or eax, 1 << 0   ; Set PE (protected mode)
    mov cr0, eax

    ; Load 64-bit GDT
    lgdt [gdt64.pointer]

    ; Jump to 64-bit code
    jmp gdt64.code:long_mode_start

.no_multiboot:
    mov al, 'M'
    jmp error

.no_cpuid:
    mov al, 'C'
    jmp error

.no_long_mode:
    mov al, 'L'
    jmp error

; Check if CPUID is supported
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    mov eax, 1
    ret
.no_cpuid:
    xor eax, eax
    ret

; Check if long mode is supported
check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    mov eax, 1
    ret
.no_long_mode:
    xor eax, eax
    ret

; Set up identity mapping and higher-half mapping
setup_page_tables:
    ; Map P4[0] -> P3 (identity mapping)
    mov eax, p3_table
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [p4_table], eax

    ; Map P4[511] -> P3 (higher-half mapping at 0xFFFFFFFF80000000)
    mov eax, p3_table
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [p4_table + 511 * 8], eax

    ; Map P3[0] -> P2 (for both identity and higher-half)
    mov eax, p2_table
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [p3_table], eax

    ; Also map P3[510] -> P2 for higher-half
    mov eax, p2_table
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [p3_table + 510 * 8], eax

    ; Map first 2MB with huge pages (P2[0])
    mov ecx, 0
.map_p2:
    mov eax, 0x200000  ; 2MB page size
    mul ecx
    or eax, PAGE_PRESENT | PAGE_WRITE | PAGE_HUGE
    mov [p2_table + ecx * 8], eax

    inc ecx
    cmp ecx, 512  ; 512 entries = 1GB
    jne .map_p2

    ret

; Error handler - prints character to VGA and hangs
error:
    mov dword [0xb8000], 0x4f524f45  ; 'ER' in red
    mov byte [0xb8004], al           ; Error code
    hlt
    jmp error

section .rodata
; 64-bit GDT
gdt64:
    dq 0  ; Null descriptor
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)  ; Code segment
.data: equ $ - gdt64
    dq (1<<44) | (1<<47)  ; Data segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 64
long_mode_start:
    ; Clear segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Restore Multiboot2 info
    mov edi, edi  ; magic (already in lower 32 bits)
    mov rsi, rsi  ; info pointer (already in lower 32 bits)

    ; Jump to higher-half kernel
    mov rax, kernel_main_higher
    jmp rax

kernel_main_higher:
    ; Call C++ kernel entry point
    extern kernel_main
    call kernel_main

    ; If kernel_main returns, halt
.hang:
    cli
    hlt
    jmp .hang
