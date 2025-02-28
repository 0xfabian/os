#pragma once

#include <mem/pmm.h>
#include <print.h>

#define HEAP_MIN_ALLOC  8
#define HEAP_MASK       7
#define HEAP_USED       1

struct Heap
{
    struct Header
    {
        usize _size;
        Header* next;
        Header* prev;

        usize size();

        void set_free();
        void set_used();
        bool is_free();

        void split(usize size);
        void combine_forward();
        void combine_backward();
    };

    Header* start;

    void init(usize pages);
    void* alloc(usize size);
    void free(void* ptr);

    void debug();
};

extern Heap heap;

void* kmalloc(usize size);
void kfree(void* ptr);