#include <mem/vmm.h>

#define KERNEL_HHDM 0xffff800000000000

VirtualMemoryManager vmm;

void VirtualMemoryManager::init()
{
    kprintf(INFO "Initializing virtual memory manager...\n");

    active_pml4 = (PML4*)(read_cr3() | KERNEL_HHDM);

    // for (int i = 0; i < 512; i++)
    // {
    //     if (active_pml4->entries[i] & PE_PRESENT)
    //     {
    //         u64 addr = (u64)i << (9 + 9 + 9 + 12);

    //         kprintf("PML4E %d (%a): %lx\n", i, addr, active_pml4->entries[i]);

    //         PDPT* pdpt = (PDPT*)((active_pml4->entries[i] & ~0xfff) | KERNEL_HHDM);

    //         for (int j = 0; j < 512; j++)
    //         {
    //             if (pdpt->entries[j] & PE_PRESENT)
    //             {
    //                 u64 addr2 = addr + ((u64)j << (9 + 9 + 12));

    //                 kprintf("    PDPTE %d (%a): %lx %f\n", j, addr2, pdpt->entries[j], pdpt->entries[j] & PE_HUGE);

    //                 if (pdpt->entries[j] & PE_HUGE)
    //                     continue;

    //                 PD* pd = (PD*)((pdpt->entries[j] & ~0xfff) | KERNEL_HHDM);

    //                 for (int k = 0; k < 512; k++)
    //                 {
    //                     if (pd->entries[k] & PE_PRESENT)
    //                     {
    //                         u64 addr3 = addr2 + ((u64)k << (9 + 12));

    //                         kprintf("        PDE %d (%a): %lx %f\n", k, addr3, pd->entries[k], pd->entries[k] & PE_HUGE);

    //                         // PT* pt = (PT*)((pd->entries[k] & ~0xfff) | KERNEL_HHDM);

    //                         // for (int l = 0; l < 512; l++)
    //                         // {
    //                         //     if (pt->entries[l] & PE_PRESENT)
    //                         //     {
    //                         //         u64 addr4 = addr3 + ((u64)l << 12);

    //                         //         kprintf("            PTE %d (%a): %lx\n", l, addr4, pt->entries[l]);
    //                         //     }
    //                         // }
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }
}

void* VirtualMemoryManager::alloc_page(u64 flags)
{
    // pmm uses only the limine usable memory regions
    // these regions are already mapped to the higher half
    // so we can just add the offset

    // this is a temporary solution
    // because we should also update the flags
    // and also handle the case where pages could be 2M or 1G

    u64 phys = (u64)pmm.alloc_page();

    if (!phys)
        return nullptr;

    // return map_page(phys | KERNEL_HHDM, phys, flags);

    return (void*)(phys | KERNEL_HHDM);
}

void* VirtualMemoryManager::alloc_pages(usize count, u64 flags)
{
    u64 phys = (u64)pmm.alloc_pages(count);

    if (!phys)
        return nullptr;

    // for (u64 off = 0; off < count * PAGE_SIZE; off += PAGE_SIZE)
    //     map_page((phys | KERNEL_HHDM) + off, phys + off, flags);

    return (void*)(phys | KERNEL_HHDM);
}

void* VirtualMemoryManager::alloc_page(u64 virt, u64 flags)
{
    u64 phys = (u64)pmm.alloc_page();

    if (!phys)
        return nullptr;

    return map_page(virt, phys, flags);
}

void* VirtualMemoryManager::alloc_pages(u64 virt, usize count, u64 flags)
{
    u64 phys = (u64)pmm.alloc_pages(count);

    if (!phys)
        return nullptr;

    for (u64 off = 0; off < count * PAGE_SIZE; off += PAGE_SIZE)
        map_page(virt + off, phys + off, flags);

    return (void*)virt;
}

void* VirtualMemoryManager::map_page(u64 virt, u64 phys, u64 flags)
{
    u32 pml4e = (virt >> 39) & 0x1ff;
    u32 pdpte = (virt >> 30) & 0x1ff;
    u32 pde = (virt >> 21) & 0x1ff;
    u32 pte = (virt >> 12) & 0x1ff;

    PML4* pml4 = active_pml4;
    PDPT* pdpt;
    PD* pd;
    PT* pt;

    if (!pml4->has(pml4e))
    {
        u64 addr = (u64)pmm.alloc_page();

        if (!addr)
            return nullptr;

        pml4->set(pml4e, addr | PE_PRESENT | PE_WRITE | PE_USER);
        pdpt = (PDPT*)(addr | KERNEL_HHDM);
        memset(pdpt, 0, PAGE_SIZE);
    }
    else
        pdpt = (PDPT*)(pml4->get(pml4e) | KERNEL_HHDM);

    if (!pdpt->has(pdpte))
    {
        u64 addr = (u64)pmm.alloc_page();

        if (!addr)
            return nullptr;

        pdpt->set(pdpte, addr | PE_PRESENT | PE_WRITE | flags);
        pd = (PD*)(addr | KERNEL_HHDM);
        memset(pd, 0, PAGE_SIZE);
    }
    else
    {
        if (pdpt->entries[pdpte] & PE_HUGE)
        {
            kprintf(PANIC "map_page: huge pdpte at %lx\n", virt);
            idle();
        }

        pd = (PD*)(pdpt->get(pdpte) | KERNEL_HHDM);
    }

    if (!pd->has(pde))
    {
        u64 addr = (u64)pmm.alloc_page();

        if (!addr)
            return nullptr;

        pd->set(pde, addr | PE_PRESENT | PE_WRITE | flags);
        pt = (PT*)(addr | KERNEL_HHDM);
        memset(pt, 0, PAGE_SIZE);
    }
    else
    {
        if (pd->entries[pde] & PE_HUGE)
        {
            kprintf(PANIC "map_page: huge pde at %lx\n", virt);
            idle();
        }

        pt = (PT*)(pd->get(pde) | KERNEL_HHDM);
    }

    pt->set(pte, phys | PE_PRESENT | flags);

    return (void*)virt;
}