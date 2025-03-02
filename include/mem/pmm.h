#pragma once

#include <requests.h>
#include <utils.h>
#include <ds/bitmap.h>
#include <print.h>

#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define PAGE_COUNT(x)   ((x + PAGE_SIZE - 1) / PAGE_SIZE)

#define KERNEL_HHDM     0xffff800000000000

struct MemoryRegion
{
    u64 base;
    u64 end;

    usize used_pages;

    BitmapView bitmap;

    void* alloc_page();
    void* alloc_pages(usize count);

    void lock_page(void* addr);
    void lock_pages(void* addr, usize count);

    void free_page(void* addr);
    void free_pages(void* addr, usize count);
};

struct PhysicalMemoryManager
{
    MemoryRegion* regions;
    usize region_count;

    usize total_pages;
    usize used_pages;

    void init();

    void* alloc_page();
    void* alloc_pages(usize count);

    void lock_page(void* addr);
    void lock_pages(void* addr, usize count);

    void free_page(void* addr);
    void free_pages(void* addr, usize count);
};

extern PhysicalMemoryManager pmm;