#pragma once

#include <types.h>

// defined in arch/gdt.cpp
#define KERNEL_CS   0x08
#define KERNEL_DS   0x10
#define USER_DS     0x1b
#define USER_CS     0x23

struct CPU
{
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 rbp;
    u64 rbx;

    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rax;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
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

static inline void cpuid(u32 code, u32* eax, u32* ebx, u32* ecx, u32* edx)
{
    asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(code));
}

static inline u8 inb(u16 port)
{
    u8 ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));

    return ret;
}

static inline u16 inw(u16 port)
{
    u16 ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "d"(port));

    return ret;
}

static inline u32 inl(u16 port)
{
    u32 ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "d"(port));

    return ret;
}

static inline void outb(u16 port, u8 val)
{
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(u16 port, u16 val)
{
    asm volatile ("outw %0, %1" : : "a"(val), "d"(port));
}

static inline void outl(u16 port, u32 val)
{
    asm volatile ("outl %0, %1" : : "a"(val), "d"(port));
}

static inline u64 read_cr3()
{
    u64 cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    return cr3;
}

static inline void write_cr3(u64 cr3)
{
    asm volatile("mov %0, %%cr3" : : "r"(cr3));
}

static inline u64 rdmsr(u64 msr)
{
    u32 eax, edx;
    asm volatile ("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");

    return ((u64)edx << 32) | eax;
}

static inline void wrmsr(u64 msr, u64 value)
{
    asm volatile ("wrmsr" : : "c"(msr), "a"(value & 0xffffffff), "d"(value >> 32) : "memory");
}

static inline u64 rdtsc()
{
    u32 lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));

    return ((u64)hi << 32) | lo;
}

void setup_syscall(u64 syscall_handler_address);

bool check_pat_support();
bool check_apic_support();
bool check_x2apic_support();