#include <tiny_os/memory/virtual_allocator.h>
#include <tiny_os/memory/physical_allocator.h>
#include <tiny_os/common/string.h>
#include <tiny_os/drivers/vga.h>
#include <tiny_os/drivers/serial.h>

namespace tiny_os::memory {

PageTable* VirtualAllocator::kernel_pml4_ = nullptr;

// External symbols from linker script
extern "C" {
    extern uint8 kernel_virtual_base;
    extern uint8 kernel_physical_end;
}

void VirtualAllocator::init() {
    drivers::kprintf("\nInitializing virtual memory...\n");
    drivers::serial_printf("Virtual memory init\n");

    // Allocate kernel PML4
    PhysicalAddress pml4_phys = PhysicalAllocator::allocate_frame();
    kernel_pml4_ = reinterpret_cast<PageTable*>(pml4_phys);
    kernel_pml4_->clear();

    drivers::serial_printf("Kernel PML4 at: 0x%lx\n", pml4_phys);

    // Identity map first 4MB (for early boot code)
    for (PhysicalAddress addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        map_page(addr, addr, PageFlags::PRESENT | PageFlags::WRITABLE);
    }

    // Map kernel to higher half (0xFFFFFFFF80000000)
    VirtualAddress kernel_virt_base =
        reinterpret_cast<VirtualAddress>(&kernel_virtual_base);
    PhysicalAddress kernel_phys_end =
        reinterpret_cast<PhysicalAddress>(&kernel_physical_end);

    drivers::serial_printf("Mapping kernel: 0x%lx (virt) -> 0x0 - 0x%lx (phys)\n",
                          kernel_virt_base, kernel_phys_end);

    // Map kernel (physical 0 -> virtual KERNEL_VIRTUAL_BASE)
    for (PhysicalAddress phys = 0; phys < kernel_phys_end; phys += PAGE_SIZE) {
        VirtualAddress virt = kernel_virt_base + phys;
        map_page(virt, phys, PageFlags::PRESENT | PageFlags::WRITABLE);
    }

    // Also map some extra space for heap (16MB)
    constexpr usize EXTRA_MAPPING = 16 * 1024 * 1024;
    for (PhysicalAddress phys = kernel_phys_end;
         phys < kernel_phys_end + EXTRA_MAPPING;
         phys += PAGE_SIZE) {
        VirtualAddress virt = kernel_virt_base + phys;
        map_page(virt, phys, PageFlags::PRESENT | PageFlags::WRITABLE);
    }

    // Load new page table
    switch_page_table(pml4_phys);

    drivers::kprintf("Virtual memory initialized\n");
    drivers::serial_printf("Virtual memory ready, CR3 = 0x%lx\n", pml4_phys);
}

void VirtualAllocator::map_page(VirtualAddress virt, PhysicalAddress phys,
                                uint64 flags) {
    auto indices = PageTableIndices::from_address(virt);

    // Get or create PDPT
    PageTable* pdpt = get_or_create_table(
        (*kernel_pml4_)[indices.pml4],
        PageFlags::PRESENT | PageFlags::WRITABLE | (flags & PageFlags::USER));

    // Get or create PD
    PageTable* pd = get_or_create_table(
        (*pdpt)[indices.pdpt],
        PageFlags::PRESENT | PageFlags::WRITABLE | (flags & PageFlags::USER));

    // Get or create PT
    PageTable* pt = get_or_create_table(
        (*pd)[indices.pd],
        PageFlags::PRESENT | PageFlags::WRITABLE | (flags & PageFlags::USER));

    // Set page table entry
    (*pt)[indices.pt].set_address(phys, flags | PageFlags::PRESENT);
}

void VirtualAllocator::unmap_page(VirtualAddress virt) {
    auto indices = PageTableIndices::from_address(virt);

    if (!(*kernel_pml4_)[indices.pml4].is_present()) return;

    auto* pdpt = reinterpret_cast<PageTable*>(
        (*kernel_pml4_)[indices.pml4].get_address());

    if (!(*pdpt)[indices.pdpt].is_present()) return;

    auto* pd = reinterpret_cast<PageTable*>(
        (*pdpt)[indices.pdpt].get_address());

    if (!(*pd)[indices.pd].is_present()) return;

    auto* pt = reinterpret_cast<PageTable*>(
        (*pd)[indices.pd].get_address());

    (*pt)[indices.pt].clear();

    // Invalidate TLB entry
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

PhysicalAddress VirtualAllocator::virt_to_phys(VirtualAddress virt) {
    auto indices = PageTableIndices::from_address(virt);

    if (!(*kernel_pml4_)[indices.pml4].is_present()) return 0;

    auto* pdpt = reinterpret_cast<PageTable*>(
        (*kernel_pml4_)[indices.pml4].get_address());

    if (!(*pdpt)[indices.pdpt].is_present()) return 0;

    auto* pd = reinterpret_cast<PageTable*>(
        (*pdpt)[indices.pdpt].get_address());

    if (!(*pd)[indices.pd].is_present()) return 0;

    auto* pt = reinterpret_cast<PageTable*>(
        (*pd)[indices.pd].get_address());

    if (!(*pt)[indices.pt].is_present()) return 0;

    return (*pt)[indices.pt].get_address() + indices.offset;
}

bool VirtualAllocator::is_mapped(VirtualAddress virt) {
    return virt_to_phys(virt) != 0;
}

PageTable* VirtualAllocator::get_kernel_pml4() {
    return kernel_pml4_;
}

void VirtualAllocator::switch_page_table(PhysicalAddress pml4_phys) {
    asm volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
}

PageTable* VirtualAllocator::get_or_create_table(PageTableEntry& entry,
                                                 uint64 flags) {
    if (entry.is_present()) {
        return reinterpret_cast<PageTable*>(entry.get_address());
    }

    // Allocate new page table
    PhysicalAddress phys = PhysicalAllocator::allocate_frame();
    auto* table = reinterpret_cast<PageTable*>(phys);
    table->clear();

    entry.set_address(phys, flags | PageFlags::PRESENT);
    return table;
}

} // namespace tiny_os::memory
