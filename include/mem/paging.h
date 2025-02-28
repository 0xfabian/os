#pragma once

#include <types.h>
#include <print.h>

#define PE_PRESENT  1
#define PE_RW       2
#define PE_USER     4
#define PE_WRITETHR 8
#define PE_NOCACHE  0x10
#define PE_ACCESSED 0x20
#define PE_DIRTY    0x40
#define PE_HUGE     0x80
#define PE_GLOBAL   0x100

#define PE_NX       0x8000000000000000

struct PML4Entry
{
    u64 value;

    bool is_present() { return value & PE_PRESENT; }
    u64 get_address()
    {
        return 0;
    }
};

struct PDPTEntry
{
    u64 value;

    bool is_present() { return value & PE_PRESENT; }
    bool is_huge() { return value & PE_HUGE; }
};

struct PDEntry
{
    u64 value;

    bool is_present() { return value & PE_PRESENT; }
    bool is_huge() { return value & PE_HUGE; }
};

struct PTEntry
{
    u64 value;

    bool is_present() { return value & PE_PRESENT; }
};

struct PML4
{
    PML4Entry entries[512];

    PML4Entry* get_entry(u64 va)
    {
        return &entries[(va >> 39) & 0x1ff];
    }
};

struct PDPT
{
    PDPTEntry entries[512];

    PDPTEntry* get_entry(u64 va)
    {
        return &entries[(va >> 30) & 0x1ff];
    }
};

struct PD
{
    PDEntry entries[512];

    PDEntry* get_entry(u64 va)
    {
        return &entries[(va >> 21) & 0x1ff];
    }
};

struct PT
{
    PTEntry entries[512];

    PTEntry* get_entry(u64 va)
    {
        return &entries[(va >> 12) & 0x1ff];
    }
};