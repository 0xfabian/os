#include <mem/pmm.h>

PhysicalMemoryManager pmm;

void* MemoryRegion::alloc_page()
{
    return alloc_pages(1);
}

void* MemoryRegion::alloc_pages(usize count)
{
    for (usize i = 0; i < bitmap.size; i++)
    {
        bool valid = true;

        for (usize j = i; j < i + count; j++)
        {
            if (j >= bitmap.size || bitmap.get(j))
            {
                valid = false;
                break;
            }
        }

        if (valid)
        {
            for (usize j = i; j < i + count; j++)
                bitmap.set(j);

            used_pages += count;

            return (void*)(base + i * PAGE_SIZE);
        }
    }

    return nullptr;
}

void MemoryRegion::lock_page(void* addr)
{
    lock_pages(addr, 1);
}

void MemoryRegion::lock_pages(void* addr, usize count)
{
    u64 index = ((u64)addr - base) >> PAGE_SHIFT;

    for (usize i = index; i < index + count; i++)
        bitmap.set(i);

    used_pages += count;
}

void MemoryRegion::free_page(void* addr)
{
    free_pages(addr, 1);
}

void MemoryRegion::free_pages(void* addr, usize count)
{
    u64 index = ((u64)addr - base) >> PAGE_SHIFT;

    for (usize i = index; i < index + count; i++)
        bitmap.clear(i);

    used_pages -= count;
}

void compute_allocator_total_size(limine_memmap_response* memmap, usize* region_count, usize* total_size)
{
    usize count = 0;
    usize bitmaps_total_size = 0;

    u64 base;
    u64 end;

    bool try_join = false;

    for (usize i = 0; i < memmap->entry_count; i++)
    {
        limine_memmap_entry* entry = memmap->entries[i];

        bool valid_type = entry->type == LIMINE_MEMMAP_USABLE;

        if (try_join)
        {
            if (valid_type && entry->base == end)
            {
                end = entry->base + entry->length;
                continue;
            }

            usize pages = PAGE_COUNT(end - base);
            usize bitmap_size = (pages + 7) / 8;

            count++;
            bitmaps_total_size += bitmap_size;

            try_join = false;
        }

        if (valid_type)
        {
            base = entry->base;
            end = entry->base + entry->length;

            try_join = true;
        }
    }

    if (try_join)
    {
        usize pages = PAGE_COUNT(end - base);
        usize bitmap_size = (pages + 7) / 8;

        count++;
        bitmaps_total_size += bitmap_size;
    }

    *region_count = count;
    *total_size = sizeof(MemoryRegion) * count + bitmaps_total_size;
}

void PhysicalMemoryManager::init()
{
    kprintf(INFO "Initializing physical memory manager...\n");

    limine_memmap_response* memmap = memmap_request.response;

    if (!memmap)
        panic("No memory map response");

    usize total_size;
    compute_allocator_total_size(memmap, &region_count, &total_size);

    void* allocator_addr = nullptr;

    for (usize i = 0; i < memmap->entry_count; i++)
    {
        limine_memmap_entry* entry = memmap->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= total_size)
        {
            allocator_addr = (void*)(entry->base | KERNEL_HHDM);
            break;
        }
    }

    if (!allocator_addr)
        panic("Failed to allocate page frame allocator");

    regions = (MemoryRegion*)allocator_addr;
    total_pages = 0;
    used_pages = 0;

    MemoryRegion* region = regions;
    u8* bitmap_addr = (u8*)(regions + region_count);

    u64 base;
    u64 end;

    bool try_join = false;

    for (usize i = 0; i < memmap->entry_count; i++)
    {
        limine_memmap_entry* entry = memmap->entries[i];

        bool valid_type = entry->type == LIMINE_MEMMAP_USABLE;

        if (try_join)
        {
            if (valid_type && entry->base == end)
            {
                end = entry->base + entry->length;
                continue;
            }

            usize pages = PAGE_COUNT(end - base);
            usize bitmap_size = (pages + 7) / 8;

            region->base = base;
            region->end = end;
            region->used_pages = 0;
            region->bitmap.from(bitmap_addr, pages);
            memset(bitmap_addr, 0, bitmap_size);

            bitmap_addr += bitmap_size;
            region++;
            total_pages += pages;

            try_join = false;
        }

        if (valid_type)
        {
            base = entry->base;
            end = entry->base + entry->length;

            try_join = true;
        }
    }

    if (try_join)
    {
        usize pages = PAGE_COUNT(end - base);
        usize bitmap_size = (pages + 7) / 8;

        region->base = base;
        region->end = end;
        region->used_pages = 0;
        region->bitmap.from(bitmap_addr, pages);
        memset(bitmap_addr, 0, bitmap_size);

        total_pages += pages;
    }

    lock_pages((void*)((u64)allocator_addr & ~KERNEL_HHDM), PAGE_COUNT(total_size));
}

void* PhysicalMemoryManager::alloc_page()
{
    for (usize i = 0; i < region_count; i++)
    {
        void* page = regions[i].alloc_page();

        if (page)
        {
            used_pages++;
            return page;
        }
    }

    return nullptr;
}

void* PhysicalMemoryManager::alloc_pages(usize count)
{
    for (usize i = 0; i < region_count; i++)
    {
        void* page = regions[i].alloc_pages(count);

        if (page)
        {
            used_pages += count;
            return page;
        }
    }

    return nullptr;
}

void PhysicalMemoryManager::lock_page(void* addr)
{
    for (usize i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (u64)addr && (u64)addr < regions[i].end)
        {
            regions[i].lock_page(addr);
            used_pages++;
            return;
        }
    }
}

void PhysicalMemoryManager::lock_pages(void* addr, usize count)
{
    for (usize i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (u64)addr && (u64)addr < regions[i].end)
        {
            regions[i].lock_pages(addr, count);
            used_pages += count;
            return;
        }
    }
}

void PhysicalMemoryManager::free_page(void* addr)
{
    for (usize i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (u64)addr && (u64)addr < regions[i].end)
        {
            regions[i].free_page(addr);
            used_pages--;
            return;
        }
    }
}

void PhysicalMemoryManager::free_pages(void* addr, usize count)
{
    for (usize i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (u64)addr && (u64)addr < regions[i].end)
        {
            regions[i].free_pages(addr, count);
            used_pages -= count;
            return;
        }
    }
}

void PhysicalMemoryManager::debug()
{
    kprintf("\e[7m %-8s %-18s    %-18s        %-12s %-12s %-12s \e[27m\n", "ENTRY", "BASE", "END", "TOTAL", "USED", "FREE");

    for (usize i = 0; i < region_count; i++)
    {
        MemoryRegion* region = &regions[i];

        kprintf(" %-8d %a    %a        %-12lu %-12lu %-12lu\n", i + 1, region->base, region->end, region->bitmap.size, region->used_pages, region->bitmap.size - region->used_pages);
    }

    usize free_pages = total_pages - used_pages;
    usize free_percent = free_pages * 100 / total_pages;
    usize used_percent = 100 - free_percent;

    kprintf("total: %lu    used: %lu (%lu%%)    free: %lu (%lu%%)\n", total_pages, used_pages, used_percent, free_pages, free_percent);
}
