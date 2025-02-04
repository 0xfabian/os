#pragma once

#include <mem/heap.h>

template <typename T>
struct ListNode
{
    T value;
    ListNode* next;
};

template <typename T>
struct List
{
    ListNode<T>* head = nullptr;
    ListNode<T>* tail = nullptr;
    size_t _size = 0;

    bool empty();
    size_t size();

    T* add(const T& value);
    void remove(T* value);
};