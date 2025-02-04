#pragma once

#include <cstdint>
#include <cstddef>

struct BitmapView
{
    size_t size;
    uint8_t* buffer;

    void from(void* data, size_t _size);
    void set(size_t bit);
    void clear(size_t bit);
    bool get(size_t bit);
};