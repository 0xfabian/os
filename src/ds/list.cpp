#include <ds/list.h>

template <typename T>
bool List<T>::empty()
{
    return _size == 0;
}