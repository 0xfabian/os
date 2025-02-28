#pragma once

#include <types.h>

struct BitmapView
{
    usize size;
    u8* buffer;

    void from(void* data, usize _size);
    void set(usize bit);
    void clear(usize bit);
    bool get(usize bit);
};