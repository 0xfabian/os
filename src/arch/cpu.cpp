#include <arch/cpu.h>

void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
    asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(code));
}

uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));

    return ret;
}

uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "d"(port));

    return ret;
}

uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "d"(port));

    return ret;
}

void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void outw(uint16_t port, uint16_t val)
{
    asm volatile ("outw %0, %1" : : "a"(val), "d"(port));
}

void outl(uint16_t port, uint32_t val)
{
    asm volatile ("outl %0, %1" : : "a"(val), "d"(port));
}

uint64_t read_cr3()
{
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

uint64_t rdmsr(uint64_t msr)
{
    uint32_t eax, edx;
    asm volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");

    return ((uint64_t)edx << 32) | eax;
}

void wrmsr(uint64_t msr, uint64_t value)
{
    asm volatile ("wrmsr" : : "c"(msr), "a"(value & 0xffffffff), "d"(value >> 32) : "memory");
}

void idle()
{
    while (true)
        asm("hlt");
}

#define MSR_EFER    0xC0000080
#define MSR_STAR    0xC0000081
#define MSR_LSTAR   0xC0000082
#define MSR_FMASK   0xC0000084

void setup_syscall(uint64_t syscall_handler_address)
{
    uint64_t efer = rdmsr(MSR_EFER);
    wrmsr(MSR_EFER, efer | 1);

    uint64_t star = (uint64_t)KERNEL_CS << 48 | (uint64_t)USER_CS << 32;
    wrmsr(MSR_STAR, star);
    wrmsr(MSR_LSTAR, syscall_handler_address);
    wrmsr(MSR_FMASK, 1 << 9);   // mask interrupt flag
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

void cli()
{
    asm volatile("cli");
}

void sti()
{
    asm volatile("sti");
}