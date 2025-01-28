#pragma once

#include <cstdint>

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