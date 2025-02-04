#include <mem/heap.h>

Heap heap;

size_t Heap::Header::size()
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

void Heap::Header::split(size_t size)
{
    Header* hdr = (Header*)((uint64_t)this + size + sizeof(Header));

    hdr->_size = this->_size - size - sizeof(Header);
    hdr->next = this->next;
    hdr->prev = this;

    if (hdr->next)
        hdr->next->prev = hdr;

    this->_size = size;
    this->next = hdr;
}

void Heap::init(size_t pages)
{
    kprintf(INFO "Initializing heap (%lu pages)...\n", pages);

    start = (Header*)((uint64_t)pfa.alloc_pages(pages) | 0xffff800000000000);

    if (!start)
        panic("Failed to allocate heap");

    start->_size = pages * PAGE_SIZE - sizeof(Header);
    start->next = nullptr;
    start->prev = nullptr;
    start->set_free();
}

void* Heap::alloc(size_t size)
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