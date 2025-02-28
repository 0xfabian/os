#pragma once

#include <types.h>
#include <print.h>

struct GDTDescriptor
{
    u16 size;
    u64 offset;
}
__attribute__((packed));

struct GDTEntry
{
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    u8 access;
    u8 flags_and_limit_high;
    u8 base_high;

    void set(u32 base, u32 limit, u8 access, u8 flags);
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