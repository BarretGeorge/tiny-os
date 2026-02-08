/* Linker script for tiny-os kernel */

/* Entry point */
ENTRY(_start)

/* Constants */
KERNEL_VIRTUAL_BASE = 0xFFFFFFFF80000000;  /* -2GB in virtual address space */
KERNEL_PHYSICAL_BASE = 0x100000;           /* 1MB physical address */

SECTIONS {
    /* Kernel starts at 1MB physical address */
    . = KERNEL_PHYSICAL_BASE;

    /* Boot section (contains multiboot header and early boot code) */
    .boot ALIGN(4K) : {
        *(.multiboot)
        *(.boot)
    }

    /* Switch to higher-half kernel virtual addresses */
    . += KERNEL_VIRTUAL_BASE;
    kernel_virtual_base = .;

    /* Text section (code) */
    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE) {
        *(.text*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE) {
        *(.rodata*)
    }

    /* Initialized data */
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE) {
        *(.data*)
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE) {
        *(COMMON)
        *(.bss*)
    }

    /* End of kernel marker */
    kernel_end = .;
    kernel_physical_end = kernel_end - KERNEL_VIRTUAL_BASE;

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note*)
        *(.comment)
    }
}
