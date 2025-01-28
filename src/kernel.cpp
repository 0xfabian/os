#include <limine.h>
#include <pfalloc.h>
#include <print.h>
#include <gdt.h>

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

uint64_t read_cr3()
{
    uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
    asm volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(code));
}

bool check_pat_support()
{
    uint32_t eax, ecx, edx, ebx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    return edx & (1 << 16);
}

bool check_apic_support()
{
    uint32_t eax, ecx, edx, ebx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    return edx & (1 << 9);
}

bool check_x2apic_support()
{
    uint32_t eax, ecx, edx, ebx;
    cpuid(1, &eax, &ebx, &ecx, &edx);

    return ecx & (1 << 21);
}

void idle()
{
    while (true)
        asm("hlt");
}

constexpr uint64_t MSR_EFER = 0xC0000080;  // Extended Feature Enable Register
constexpr uint64_t MSR_STAR = 0xC0000081;  // Segment selectors
constexpr uint64_t MSR_LSTAR = 0xC0000082; // Syscall handler entry point
constexpr uint64_t MSR_FMASK = 0xC0000084; // Syscall flag mask

void wrmsr(uint64_t msr, uint64_t value) {
    asm volatile (
        "wrmsr"
        :
    : "c"(msr), "a"(value & 0xFFFFFFFF), "d"(value >> 32)
        : "memory"
        );
}

uint64_t rdmsr(uint64_t msr) {
    uint32_t lo, hi;
    asm volatile (
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
        : "memory"
        );
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

void setup_syscall(uint64_t syscall_handler_address, uint16_t kernel_cs, uint16_t user_cs) {
    // Enable SYSCALL/SYSRET instructions
    uint64_t efer = rdmsr(MSR_EFER);
    efer |= 1; // Enable SYSCALL
    wrmsr(MSR_EFER, efer);

    // Set up STAR MSR for segment selectors
    uint64_t star_value = (static_cast<uint64_t>(kernel_cs) << 48) |
        (static_cast<uint64_t>(user_cs) << 32);
    wrmsr(MSR_STAR, star_value);

    // Set up LSTAR MSR for syscall entry point
    wrmsr(MSR_LSTAR, syscall_handler_address);

    // Set up FMASK MSR to mask interrupt flag (IF)
    wrmsr(MSR_FMASK, 1 << 9);
}

void syscall_handler()
{
    kprintf("syscall\n");
    idle();
}

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

    for (size_t i = 0; i < pfa.region_count; i++)
    {
        MemoryRegion* region = &pfa.regions[i];

        kprintf("region %d: %a-%a    total: %d    used: %d\n", i, region->base, region->end, region->bitmap.size, region->used_pages);
    }

    gdt.init();

    kprintf(INFO "gdt initialized\n");

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

    // if (try_join)
    //     kprintf("%a-%a\n", lower, upper + 1);

    // setup_syscall((uint64_t)syscall_handler, 0x08, 0x18);

    // asm volatile("syscall");

    uint64_t apic_base = rdmsr(0x1b);

    kprintf("apic: %f\n", check_apic_support());
    kprintf("x2apic: %f\n", check_x2apic_support());

    kprintf("apic_base: %lx\n", apic_base);

    idle();
}