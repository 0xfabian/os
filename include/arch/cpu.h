#pragma once

#include <cstdint>

#define KERNEL_CS   0x08
#define KERNEL_SS   0x10
#define USER_CS     0x1b
#define USER_SS     0x23

void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
void outl(uint16_t port, uint32_t val);

uint64_t read_cr3();

uint64_t rdmsr(uint64_t msr);
void wrmsr(uint64_t msr, uint64_t value);

void idle();
void setup_syscall(uint64_t syscall_handler_address);

bool check_pat_support();
bool check_apic_support();
bool check_x2apic_support();

void cli();
void sti();

struct CPU
{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rip;
    uint64_t rflags;
};