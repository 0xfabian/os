#pragma once

#include <cstdint>

// defined in arch/gdt.cpp
#define KERNEL_CS   0x08
#define KERNEL_DS   0x10
#define USER_DS     0x1b
#define USER_CS     0x23

struct CPU
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbp;
    uint64_t rbx;

    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

static inline void cli()
{
    asm volatile("cli");
}

static inline void sti()
{
    asm volatile("sti");
}

static inline void idle()
{
    while (true)
        asm volatile("hlt");
}

static inline void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
    asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(code));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));

    return ret;
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "d"(port));

    return ret;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "d"(port));

    return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val)
{
    asm volatile ("outw %0, %1" : : "a"(val), "d"(port));
}

static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile ("outl %0, %1" : : "a"(val), "d"(port));
}

static inline uint64_t read_cr3()
{
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

static inline uint64_t rdmsr(uint64_t msr)
{
    uint32_t eax, edx;
    asm volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");

    return ((uint64_t)edx << 32) | eax;
}

static inline void wrmsr(uint64_t msr, uint64_t value)
{
    asm volatile ("wrmsr" : : "c"(msr), "a"(value & 0xffffffff), "d"(value >> 32) : "memory");
}

void setup_syscall(uint64_t syscall_handler_address);

bool check_pat_support();
bool check_apic_support();
bool check_x2apic_support();