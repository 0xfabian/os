#include <mem/heap.h>

Heap heap;

usize Heap::Header::size()
{
    return _size & ~HEAP_MASK;
}

void Heap::Header::set_free()
{
    _size &= ~HEAP_USED;
}

void Heap::Header::set_used()
{
    _size |= HEAP_USED;
}

bool Heap::Header::is_free()
{
    return (_size & HEAP_USED) == 0;
}

void Heap::Header::combine_forward()
{
    if (!next || !next->is_free())
        return;

    _size += next->size() + sizeof(Header);
    next = next->next;

    if (next)
        next->prev = this;
}

void Heap::Header::combine_backward()
{
    if (!prev || !prev->is_free())
        return;

    prev->combine_forward();
}

void Heap::Header::split(usize size)
{
    Header* hdr = (Header*)((u64)this + size + sizeof(Header));

    hdr->_size = this->_size - size - sizeof(Header);
    hdr->next = this->next;
    hdr->prev = this;

    if (hdr->next)
        hdr->next->prev = hdr;

    this->_size = size;
    this->next = hdr;
}

void Heap::init(usize pages)
{
    kprintf(INFO "Initializing heap (%lu pages)...\n", pages);

    start = (Header*)vmm.alloc_pages(pages, PE_WRITE);

    if (!start)
        panic("Failed to allocate heap");

    start->_size = pages * PAGE_SIZE - sizeof(Header);
    start->next = nullptr;
    start->prev = nullptr;
    start->set_free();
}

void* Heap::alloc(usize size)
{
    if (size == 0 || size & HEAP_MASK)
        size = (size & ~HEAP_MASK) + HEAP_MIN_ALLOC;

    Header* hdr = start;

    while (hdr)
    {
        if (hdr->is_free() && hdr->size() >= size)
        {
            if (hdr->size() >= size + sizeof(Header) + HEAP_MIN_ALLOC)
                hdr->split(size);

            hdr->set_used();

            return (void*)(hdr + 1);
        }

        hdr = hdr->next;
    }

    return nullptr;
}

void Heap::free(void* ptr)
{
    if (!ptr)
        return;

    Header* hdr = (Header*)ptr - 1;

    hdr->set_free();
    hdr->combine_forward();
    hdr->combine_backward();
}

void Heap::debug()
{
    Header* hdr = start;

    usize free = 0;
    usize used = 0;
    usize entries = 1;

    kprintf("\e[7m %-8s %-8s %-14s %-18s    %-18s    %-18s \e[27m\n", "ENTRY", "TYPE", "SIZE", "ADDRESS", "NEXT", "PREV");

    while (hdr)
    {
        if (hdr->is_free())
            free += hdr->size();
        else
            used += hdr->size();

        kprintf(" %-8lu \e[9%dm%-8s\e[m %-14lu %p    %p    %p\n", entries, 1 + hdr->is_free(), hdr->is_free() ? "free" : "used", hdr->size(), hdr, hdr->next, hdr->prev);

        if (hdr->next && hdr->next->prev != hdr)
            kprintf(WARN "this->next->prev != this\n");

        if (hdr->prev && hdr->prev->next != hdr)
            kprintf(WARN "this->prev->next != this\n");

        entries++;

        hdr = hdr->next;
    }

    usize total = used + free + entries * sizeof(Header);
    usize free_percent = free * 100 / total;
    usize used_percent = 100 - free_percent;

    kprintf("total: %lu    used: %lu (%lu%%)    free: %lu (%lu%%)\n", total, used, used_percent, free, free_percent);
}

void* kmalloc(usize size)
{
    return heap.alloc(size);
}

void kfree(void* ptr)
{
    heap.free(ptr);
}
