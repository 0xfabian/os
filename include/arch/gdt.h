#pragma once

#include <cstdint>
#include <print.h>

struct GDTDescriptor
{
    uint16_t size;
    uint64_t offset;
}
__attribute__((packed));

struct GDTEntry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags_and_limit_high;
    uint8_t base_high;

    void set(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
}
__attribute__((packed));

struct GDT
{
    GDTEntry null;
    GDTEntry kernel_code;
    GDTEntry kernel_data;
    GDTEntry user_data;
    GDTEntry user_code;

    void init();
}
__attribute__((packed));

extern GDT gdt;