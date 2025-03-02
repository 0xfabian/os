#pragma once

#include <types.h>
#include <print.h>

#define PE_PRESENT  1
#define PE_WRITE    2
#define PE_USER     4
#define PE_HUGE     0x80

struct PML4
{
    u64 entries[512];

    bool has(u32 index)
    {
        return entries[index] & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index] & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index] = value;
    }
};

struct PDPT
{
    u64 entries[512];

    bool has(u32 index)
    {
        return entries[index] & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index] & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index] = value;
    }
};

struct PD
{
    u64 entries[512];

    bool has(u32 index)
    {
        return entries[index] & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index] & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index] = value;
    }
};

struct PT
{
    u64 entries[512];

    bool has(u32 index)
    {
        return entries[index] & PE_PRESENT;
    }

    u64 get(u32 index)
    {
        // should use another mask here because top bits can be set
        return entries[index] & ~0xfff;
    }

    void set(u32 index, u64 value)
    {
        entries[index] = value;
    }
};