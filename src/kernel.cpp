#include <limine.h>
#include <pfalloc.h>
#include <print.h>

// #define KB(x) ((x) / 1024)
// #define MB(x) ((x) / 1048576)
// #define GB(x) ((x) / 1073741824) 

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// uint64_t rdmsr(uint32_t msr)
// {
//     uint32_t low, high;

//     asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));

//     return ((uint64_t)high << 32) | low;
// }

// uint64_t read_cr0()
// {
//     uint64_t val;
//     asm volatile("mov %%cr0, %0" : "=r"(val));
//     return val;
// }

// uint64_t read_cr2()
// {
//     uint64_t val;
//     asm volatile("mov %%cr2, %0" : "=r"(val));
//     return val;
// }

uint64_t read_cr3()
{
    uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

// uint64_t read_cr4()
// {
//     uint64_t val;
//     asm volatile("mov %%cr4, %0" : "=r"(val));
//     return val;
// }

// void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
// {
//     asm volatile("cpuid"
//         : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
//         : "a"(code));
// }

// bool check_pat_support()
// {
//     uint32_t eax, ecx, edx, ebx;
//     cpuid(1, &eax, &ebx, &ecx, &edx);

//     return edx & (1 << 16);
// }

void idle()
{
    while (true)
        asm("hlt");
}

// uint64_t saved_pat;

// const char* pat_str[] =
// {
//     "uc",
//     "wc",
//     "?",
//     "?",
//     "wt",
//     "wp",
//     "wb",
//     "uc-"
// };

// void print_page(uint64_t page)
// {
//     uint8_t* pat_entries = (uint8_t*)&saved_pat;
//     int pat = (page >> 7) & 1;
//     int pcd = (page >> 4) & 1;
//     int pwt = (page >> 3) & 1;

//     int index = pat << 2 | pcd << 1 | pwt;

//     kprintf("%p %s", page & ~0xfff, pat_str[pat_entries[index]]);
// }

extern "C" void kmain(void)
{
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
        idle();

    limine_framebuffer_response* framebuffer_response = framebuffer_request.response;

    if (!framebuffer_response || framebuffer_response->framebuffer_count == 0)
        idle();

    default_fb.init(framebuffer_response->framebuffers[0]);

    limine_memmap_response* memmap_response = memmap_request.response;

    if (!memmap_response || !pfa.init(memmap_response))
        idle();

    size_t count = PAGE_COUNT(default_fb.width * default_fb.height * sizeof(uint32_t));
    backbuffer = (uint32_t*)((uint64_t)pfa.alloc_pages(count) | 0xffff800000000000);

    if (!backbuffer)
        idle();

    fbterm.init();

    kprintf(INFO "%dx%d fbterm initialized\n", fbterm.width, fbterm.height);
    kprintf(INFO "backbuffer at %a\n", backbuffer);
    kprintf(INFO "total pages: %d\n", pfa.total_pages);
    kprintf(INFO "used pages: %d\n", pfa.used_pages);

    kprintf("\n");

    for (size_t i = 0; i < pfa.region_count; i++)
    {
        MemoryRegion* region = &pfa.regions[i];

        kprintf("region %d: %a-%a    total: %d    used: %d\n", i, region->base, region->end, region->total_pages, region->used_pages);
    }

    for (uint64_t i = 0; i < memmap_response->entry_count; i++)
    {
        limine_memmap_entry* entry = memmap_response->entries[i];

        const char* type_str[] =
        {
            "\e[92musable\e[m",
            "reserved",
            "ACPI reclaimable",
            "ACPI NVS",
            "bad memory",
            "\e[92mbootloader reclaimable\e[m",
            "executable and modules",
            "framebuffer"
        };

        kprintf("%a-%a %s (%d pages)\n", entry->base, entry->base + entry->length, type_str[entry->type], entry->length / 4096);
    }

    // uint64_t* pml4 = (uint64_t*)(read_cr3() | 0xffff800000000000);

    // bool try_join = false;
    // uint64_t lower;
    // uint64_t upper;
    // int n = 0;

    // kprintf("pml4: %a\n", pml4);

    // for (int i = 0; i < 512; i++)
    // {
    //     if (pml4[i] & 1)
    //     {
    //         uint64_t lower4 = (uint64_t)i << (9 + 9 + 9 + 12);
    //         uint64_t upper4 = lower4 | 0x7fffffffff;

    //         // kprintf("pml4[%d]: %a-%a\n", i, lower4, upper4);
    //         // kprintf("pml4[%d]: %a\n", i, pml4[i]);

    //         uint64_t* pdpt = (uint64_t*)((pml4[i] & ~0xfff) | 0xffff800000000000);

    //         for (int j = 0; j < 512; j++)
    //         {
    //             if (pdpt[j] & 1)
    //             {
    //                 uint64_t lower3 = lower4 | (uint64_t)j << (9 + 9 + 12);
    //                 uint64_t upper3 = lower3 | 0x3fffffff;

    //                 // kprintf("    pdpt[%d]: %a-%a\n", j, lower3, upper3);
    //                 uint64_t pdpte = pdpt[j];
    //                 bool large = pdpte & (1 << 7);

    //                 // kprintf("    pdpt[%d]: %a %s\n", j, pdpt[j], large ? "large" : "");

    //                 uint64_t* pdt = (uint64_t*)((pdpt[j] & ~0xfff) | 0xffff800000000000);

    //                 if (large)
    //                 {
    //                     // kprintf("big pdpte\n");

    //                     if (!try_join)
    //                     {
    //                         lower = lower3;
    //                         upper = upper3;

    //                         try_join = true;
    //                     }
    //                     else
    //                     {
    //                         if (lower3 == upper + 1)
    //                             upper = upper3;
    //                         else
    //                         {
    //                             kprintf("%a-%a\n", lower, upper + 1);

    //                             lower = lower3;
    //                             upper = upper3;
    //                         }
    //                     }

    //                     continue;
    //                 }

    //                 for (int k = 0; k < 512; k++)
    //                 {
    //                     if (pdt[k] & 1)
    //                     {
    //                         uint64_t lower2 = lower3 | (uint64_t)k << (9 + 12);
    //                         uint64_t upper2 = lower2 | 0x1fffff;

    //                         // kprintf("        pdt[%d]: %a-%a\n", k, lower2, upper2);
    //                         uint64_t pde = pdt[k];
    //                         bool large = pde & (1 << 7);

    //                         // kprintf("        pdt[%d]: %a %s\n", k, pdt[k], large ? "large" : "");

    //                         uint64_t* pt = (uint64_t*)((pdt[k] & ~0xfff) | 0xffff800000000000);

    //                         if (large)
    //                         {
    //                             if (!try_join)
    //                             {
    //                                 lower = lower2;
    //                                 upper = upper2;

    //                                 try_join = true;
    //                             }
    //                             else
    //                             {
    //                                 if (lower2 == upper + 1)
    //                                     upper = upper2;
    //                                 else
    //                                 {
    //                                     kprintf("%a-%a\n", lower, upper + 1);

    //                                     lower = lower2;
    //                                     upper = upper2;
    //                                 }
    //                             }

    //                             continue;
    //                         }

    //                         for (int l = 0; l < 512; l++)
    //                         {
    //                             if (pt[l] & 1)
    //                             {
    //                                 uint64_t lower1 = lower2 | (uint64_t)l << 12;
    //                                 uint64_t upper1 = lower1 | 0xfff;

    //                                 // kprintf("            pt[%d]: %a-%a\n", l, lower1, upper1);
    //                                 // kprintf("            pt[%d]: %a\n", l, pt[l]);


    //                                 //     if (i == 256 && j == 0 && k == 5 && l >= 110 && l <= 115)
    //                                 //         kprintf("%a %d\n", lower, l);

    //                                 //     // if (lower == 0x800000a6e000)
    //                                 //     //     kprintf("gasit %d %d %d %d\n", i, j, k, l);

    //                                 if (!try_join)
    //                                 {
    //                                     lower = lower1;
    //                                     upper = upper1;

    //                                     try_join = true;
    //                                 }
    //                                 else
    //                                 {
    //                                     if (lower1 == upper + 1)
    //                                         upper = upper1;
    //                                     else
    //                                     {
    //                                         kprintf("%a-%a\n", lower, upper + 1);

    //                                         lower = lower1;
    //                                         upper = upper1;
    //                                     }
    //                                 }
    //                             }
    //                             // else
    //                             // {
    //                             //     if (pt[l] != 0)
    //                             //         kprintf(FATAL "%a %a %a: %lx missing ?\n", pml4[i], pdpt[j], pdt[k], pt[l]);
    //                             // }
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }

    idle();
}