#pragma once

#include <mem/pmm.h>
#include <mem/paging.h>

struct VirtualMemoryManager
{
    PML4* active_pml4;

    void init();

    void* alloc_page(u64 flags);
    void* alloc_pages(usize count, u64 flags);

    // i don't know how to handle huge pages yet
    // so virt should be an unmapped user space address

    void* alloc_page(u64 virt, u64 flags);
    void* alloc_pages(u64 virt, usize count, u64 flags);

    void* alloc_page(PML4* pml4, u64 virt, u64 flags);
    void* alloc_page_and_copy(PML4* pml4, u64 virt, u64 flags);
    void* alloc_pages(PML4* pml4, u64 virt, usize count, u64 flags);

    void* map_page(PML4* pml4, u64 virt, u64 phys, u64 flags);

    PML4* make_user_page_table();
    void switch_pml4(PML4* pml4);

    u64 virt_to_phys(PML4* pml4, u64 virt);
    u64 virt_to_kernel(PML4* pml4, u64 virt);
};

extern VirtualMemoryManager vmm;