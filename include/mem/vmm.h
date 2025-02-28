#pragma once

#include <mem/pmm.h>
#include <mem/paging.h>

struct VirtualMemoryManager
{
    PML4* active_pml4;

    void init();

    u64 phys_to_virt(u64 addr);
    u64 virt_to_phys(u64 addr);
    u64* get_page_value(u64 addr);
};

extern VirtualMemoryManager vmm;