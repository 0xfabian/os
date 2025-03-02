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

    void* map_page(u64 virt, u64 phys, u64 flags);
};

extern VirtualMemoryManager vmm;