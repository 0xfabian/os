#include <ds/bitmap.h>

void BitmapView::from(void* data, usize _size)
{
    buffer = (u8*)data;
    size = _size;
}

void BitmapView::set(usize index)
{
    if (index >= size)
        return;

    buffer[index / 8] |= (1 << (index % 8));
}

void BitmapView::clear(usize index)
{
    if (index >= size)
        return;

    buffer[index / 8] &= ~(1 << (index % 8));
}

bool BitmapView::get(usize index)
{
    if (index >= size)
        return false;

    return buffer[index / 8] & (1 << (index % 8));
}
