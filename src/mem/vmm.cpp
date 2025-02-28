#include <mem/vmm.h>

#define KERNEL_HHDM 0xffff800000000000

VirtualMemoryManager vmm;

void VirtualMemoryManager::init()
{
    pmm.init();

    PML4* pml4 = (PML4*)phys_to_virt(read_cr3());

    return;

    for (usize i = 0; i < 512; i++)
    {
        if (pml4->entries[i].is_present())
        {
            kprintf("PML4[%d] = %a\n", i, pml4->entries[i].value);

            PDPT* pdpt = (PDPT*)phys_to_virt(pml4->entries[i].value & ~0xfff);

            for (usize j = 0; j < 512; j++)
            {
                if (pdpt->entries[j].is_present())
                {
                    kprintf("    PDPT[%d] = %a\n", j, pdpt->entries[j].value);

                    if (pdpt->entries[j].is_huge())
                        continue;

                    PD* pd = (PD*)phys_to_virt(pdpt->entries[j].value & ~0xfff);

                    for (usize k = 0; k < 512; k++)
                    {
                        if (pd->entries[k].is_present())
                        {
                            kprintf("        PD[%d] = %a\n", k, pd->entries[k].value);

                            if (pd->entries[k].is_huge())
                                continue;

                            PT* pt = (PT*)phys_to_virt(pd->entries[k].value & ~0xfff);

                            for (usize l = 0; l < 512; l++)
                            {
                                if (pt->entries[l].is_present())
                                {
                                    kprintf("            PT[%d] = %a\n", l, pt->entries[l].value);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#define PML4_INDEX(va) ((va >> 39) & 0x1ff)
#define PDPT_INDEX(va) ((va >> 30) & 0x1ff)
#define PD_INDEX(va) ((va >> 21) & 0x1ff)
#define PT_INDEX(va) ((va >> 12) & 0x1ff)

u64 VirtualMemoryManager::phys_to_virt(u64 addr)
{
    return addr + KERNEL_HHDM;
}

u64 VirtualMemoryManager::virt_to_phys(u64 addr)
{
    return 0;

    // u64 pml4_addr = read_cr3();
    // PML4* pml4 = (PML4*)phys_to_virt(pml4_addr);
    // PML4Entry* pml4_entry = pml4->entries + PML4_INDEX(addr);

    // if (!pml4_entry->is_present())
    //     return 0;

    // u64 pdpt_addr = pml4_entry->get_address();
    // PDPT* pdpt = (PDPT*)phys_to_virt(pdpt_addr);
    // PDPTEntry* pdpt_entry = pdpt->entries + PDPT_INDEX(addr);

    // if (!pdpt_entry->is_present())
    //     return 0;

    // if (pdpt_entry->is_huge())
    //     return (void*)()

    //     void* pd_addr = (void*)

    //     if (pdpt_entry.value & PE_HUGE)
    //         return (void*)((pdpt_entry.value & ~0x1fffff) + (va & 0x1fffff));

    // PD* pd = (PD*)phys_to_virt((void*)(pdpt_entry.value & ~0xfff));

    // PDEntry pd_entry = pd->entries[(va >> 21) & 0x1ff];

    // if (pd_entry.value & PE_PRESENT == 0)
    //     return nullptr;

    // if (pd_entry.value & PE_HUGE)
    //     return (void*)((pd_entry.value & ~0x1fffff) + (va & 0x1fffff));

    // PT* pt = (PT*)phys_to_virt((void*)(pd_entry.value & ~0xfff));

    // PTEntry pt_entry = pt->entries[(va >> 12) & 0x1ff];

    // if (pt_entry.value & PE_PRESENT == 0)
    //     return nullptr;

    // return (void*)((pt_entry.value & ~0xfff) + (va & 0xfff));
}

u64* VirtualMemoryManager::get_page_value(u64 addr)
{
    u64 pml4_addr = read_cr3();
    PML4* pml4 = (PML4*)phys_to_virt(pml4_addr);

    PML4Entry* pml4_entry = pml4->entries + PML4_INDEX(addr);

    if (!pml4_entry->is_present())
        return nullptr;

    u64 pdpt_addr = pml4_entry->value & ~0xfff;
    PDPT* pdpt = (PDPT*)phys_to_virt(pdpt_addr);

    PDPTEntry* pdpt_entry = pdpt->entries + PDPT_INDEX(addr);

    if (!pdpt_entry->is_present())
        return nullptr;

    if (pdpt_entry->is_huge())
        return &pdpt_entry->value;

    u64 pd_addr = pdpt_entry->value & ~0xfff;
    PD* pd = (PD*)phys_to_virt(pd_addr);

    PDEntry* pd_entry = pd->entries + PD_INDEX(addr);

    if (!pd_entry->is_present())
        return nullptr;

    if (pd_entry->is_huge())
        return &pd_entry->value;

    u64 pt_addr = pd_entry->value & ~0xfff;
    PT* pt = (PT*)phys_to_virt(pt_addr);

    PTEntry* pt_entry = pt->entries + PT_INDEX(addr);

    if (!pt_entry->is_present())
        return nullptr;

    return &pt_entry->value;
}