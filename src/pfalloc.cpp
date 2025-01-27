#include <pfalloc.h>

PageFrameAllocator pfa;

void* MemoryRegion::alloc_page()
{
    return alloc_pages(1);
}

void* MemoryRegion::alloc_pages(size_t count)
{
    for (size_t i = 0; i < bitmap.size; i++)
    {
        bool valid = true;

        for (size_t j = i; j < i + count; j++)
        {
            if (j >= bitmap.size || bitmap.get(j))
            {
                valid = false;
                break;
            }
        }

        if (valid)
        {
            for (size_t j = i; j < i + count; j++)
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

void MemoryRegion::lock_pages(void* addr, size_t count)
{
    uint64_t index = ((uint64_t)addr - base) >> PAGE_SHIFT;

    for (size_t i = index; i < index + count; i++)
        bitmap.set(i);

    used_pages += count;
}

void MemoryRegion::free_page(void* addr)
{
    free_pages(addr, 1);
}

void MemoryRegion::free_pages(void* addr, size_t count)
{
    uint64_t index = ((uint64_t)addr - base) >> PAGE_SHIFT;

    for (size_t i = index; i < index + count; i++)
        bitmap.clear(i);

    used_pages -= count;
}

void compute_allocator_total_size(limine_memmap_response* memmap, size_t* region_count, size_t* total_size)
{
    size_t count = 0;
    size_t bitmaps_total_size = 0;

    uint64_t base;
    uint64_t end;

    bool try_join = false;

    for (size_t i = 0; i < memmap->entry_count; i++)
    {
        limine_memmap_entry* entry = memmap->entries[i];

        bool valid_type = entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;

        if (try_join)
        {
            if (valid_type && entry->base == end)
            {
                end = entry->base + entry->length;
                continue;
            }

            size_t pages = PAGE_COUNT(end - base);
            size_t bitmap_size = (pages + 7) / 8;

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
        size_t pages = PAGE_COUNT(end - base);
        size_t bitmap_size = (pages + 7) / 8;

        count++;
        bitmaps_total_size += bitmap_size;
    }

    *region_count = count;
    *total_size = sizeof(MemoryRegion) * count + bitmaps_total_size;
}

bool PageFrameAllocator::init(limine_memmap_response* memmap)
{
    size_t total_size;
    compute_allocator_total_size(memmap, &region_count, &total_size);

    void* allocator_addr = nullptr;

    for (size_t i = 0; i < memmap->entry_count; i++)
    {
        limine_memmap_entry* entry = memmap->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= total_size)
        {
            allocator_addr = (void*)(entry->base | 0xffff800000000000);
            break;
        }
    }

    if (!allocator_addr)
        return false;

    regions = (MemoryRegion*)allocator_addr;
    total_pages = 0;
    used_pages = 0;

    MemoryRegion* region = regions;
    uint8_t* bitmap_addr = (uint8_t*)(regions + region_count);

    uint64_t base;
    uint64_t end;

    bool try_join = false;

    for (size_t i = 0; i < memmap->entry_count; i++)
    {
        limine_memmap_entry* entry = memmap->entries[i];

        bool valid_type = entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;

        if (try_join)
        {
            if (valid_type && entry->base == end)
            {
                end = entry->base + entry->length;
                continue;
            }

            size_t pages = PAGE_COUNT(end - base);
            size_t bitmap_size = (pages + 7) / 8;

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
        size_t pages = PAGE_COUNT(end - base);
        size_t bitmap_size = (pages + 7) / 8;

        region->base = base;
        region->end = end;
        region->used_pages = 0;
        region->bitmap.from(bitmap_addr, pages);
        memset(bitmap_addr, 0, bitmap_size);

        total_pages += pages;
    }

    lock_pages((void*)((uint64_t)allocator_addr & ~0xffff800000000000), PAGE_COUNT(total_size));

    return true;
}

void* PageFrameAllocator::alloc_page()
{
    for (size_t i = 0; i < region_count; i++)
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

void* PageFrameAllocator::alloc_pages(size_t count)
{
    for (size_t i = 0; i < region_count; i++)
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

void PageFrameAllocator::lock_page(void* addr)
{
    for (size_t i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (uint64_t)addr && (uint64_t)addr < regions[i].end)
        {
            regions[i].lock_page(addr);
            used_pages++;
            return;
        }
    }
}

void PageFrameAllocator::lock_pages(void* addr, size_t count)
{
    for (size_t i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (uint64_t)addr && (uint64_t)addr < regions[i].end)
        {
            regions[i].lock_pages(addr, count);
            used_pages += count;
            return;
        }
    }
}

void PageFrameAllocator::free_page(void* addr)
{
    for (size_t i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (uint64_t)addr && (uint64_t)addr < regions[i].end)
        {
            regions[i].free_page(addr);
            used_pages--;
            return;
        }
    }
}

void PageFrameAllocator::free_pages(void* addr, size_t count)
{
    for (size_t i = 0; i < region_count; i++)
    {
        if (regions[i].base <= (uint64_t)addr && (uint64_t)addr < regions[i].end)
        {
            regions[i].free_pages(addr, count);
            used_pages -= count;
            return;
        }
    }
}