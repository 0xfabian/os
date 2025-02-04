#include <ds/bitmap.h>

void BitmapView::from(void* data, size_t _size)
{
    buffer = (uint8_t*)data;
    size = _size;
}

void BitmapView::set(size_t index)
{
    if (index >= size)
        return;

    buffer[index / 8] |= (1 << (index % 8));
}

void BitmapView::clear(size_t index)
{
    if (index >= size)
        return;

    buffer[index / 8] &= ~(1 << (index % 8));
}

bool BitmapView::get(size_t index)
{
    if (index >= size)
        return false;

    return buffer[index / 8] & (1 << (index % 8));
}