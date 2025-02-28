#pragma once

#include <types.h>
#include <print.h>

struct TSS
{
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
}
__attribute__((packed));

struct TSSDescriptor
{
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    u8 access;
    u8 flags_and_limit_high;
    u8 base_high;
    u32 base_high2;
    u32 reserved;

    void set(u64 base, u32 limit, u8 access, u8 flags);
}
__attribute__((packed));

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
    TSSDescriptor tss_desc;

    void init();
}
__attribute__((packed));

extern GDT gdt;
extern TSS tss;