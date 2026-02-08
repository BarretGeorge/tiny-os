#pragma once

#include <tiny_os/common/types.h>
#include <tiny_os/memory/page_table.h>

namespace tiny_os::memory {

class VirtualAllocator {
public:
    static void init();

    // Map a virtual address to a physical address
    static void map_page(VirtualAddress virt, PhysicalAddress phys, uint64 flags);

    // Unmap a virtual address
    static void unmap_page(VirtualAddress virt);

    // Get physical address for virtual address
    static PhysicalAddress virt_to_phys(VirtualAddress virt);

    // Check if page is mapped
    static bool is_mapped(VirtualAddress virt);

    // Get current page table
    static PageTable* get_kernel_pml4();

    // Switch page table (load CR3)
    static void switch_page_table(PhysicalAddress pml4_phys);

private:
    static PageTable* kernel_pml4_;

    // Ensure page table exists at given level
    static PageTable* ensure_table(PageTable* table, usize index, uint64 flags);

    // Get or create page table
    static PageTable* get_or_create_table(PageTableEntry& entry, uint64 flags);
};

} // namespace tiny_os::memory
