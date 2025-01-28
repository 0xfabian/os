#pragma once

#include <limine.h>
#include <utils.h>
#include <bitmap.h>

#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define PAGE_COUNT(x)   ((x + PAGE_SIZE - 1) / PAGE_SIZE)

struct MemoryRegion
{
    uint64_t base;
    uint64_t end;

    size_t used_pages;

    BitmapView bitmap;

    void* alloc_page();
    void* alloc_pages(size_t count);

    void lock_page(void* addr);
    void lock_pages(void* addr, size_t count);

    void free_page(void* addr);
    void free_pages(void* addr, size_t count);
};

struct PageFrameAllocator
{
    MemoryRegion* regions;
    size_t region_count;

    size_t total_pages;
    size_t used_pages;

    bool init(limine_memmap_response* memmap);

    void* alloc_page();
    void* alloc_pages(size_t count);

    void lock_page(void* addr);
    void lock_pages(void* addr, size_t count);

    void free_page(void* addr);
    void free_pages(void* addr, size_t count);
};

extern PageFrameAllocator pfa;