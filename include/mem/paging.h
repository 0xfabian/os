#pragma once

#include <types.h>
#include <print.h>

#define PE_PRESENT  1
#define PE_WRITE    2
#define PE_USER     4
#define PE_HUGE     0x80

struct PML4Entry
{
    u64 value;

    u64 get_address()
    {
        return 0;
    }
};

struct PDPTEntry
{
    u64 value;

    bool is_huge() { return value & PE_HUGE; }
};

struct PDEntry
{
    u64 value;

    bool is_huge() { return value & PE_HUGE; }
};

struct PTEntry
{
    u64 value;
};

struct PML4
{
    PML4Entry entries[512];

    bool has(u32 index)
    {
        return entries[index].value & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index].value & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index].value = value;
    }
};

struct PDPT
{
    PDPTEntry entries[512];

    bool has(u32 index)
    {
        return entries[index].value & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index].value & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index].value = value;
    }
};

struct PD
{
    PDEntry entries[512];

    bool has(u32 index)
    {
        return entries[index].value & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index].value & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index].value = value;
    }
};

struct PT
{
    PTEntry entries[512];

    bool has(u32 index)
    {
        return entries[index].value & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index].value & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index].value = value;
    }
};