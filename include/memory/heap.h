#pragma once

#include <memory/pfalloc.h>

#define HEAP_MIN_ALLOC  8
#define HEAP_MASK       7
#define HEAP_USED       1

struct Heap
{
    struct Header
    {
        size_t _size;
        Header* next;
        Header* prev;

        size_t size();

        void set_free();
        void set_used();
        bool is_free();

        void split(size_t size);
        void combine_forward();
        void combine_backward();
    };

    Header* start;

    void init(size_t pages);
    void* alloc(size_t size);
    void free(void* ptr);
};

extern Heap heap;