#pragma once

#include <mem/heap.h>

template <typename T>
struct ListNode
{
    T value;
    ListNode* next;

    ListNode(const T& value) : value(value), next(nullptr) {}
};

template <typename T>
struct List
{
    struct Iterator
    {
        ListNode<T>* node;

        Iterator(ListNode<T>* node) : node(node) {}

        T& operator*() { return node->value; }
        Iterator& operator++() { node = node->next; return *this; }
        bool operator!=(const Iterator& other) { return node != other.node; }
    };

    ListNode<T>* head = nullptr;
    ListNode<T>* tail = nullptr;
    size_t _size = 0;

    bool empty() { return _size == 0; };
    size_t size() { return _size; };

    T* add(const T& value)
    {
        ListNode<T>* node = new ListNode<T>(value);

        if (!head)
        {
            head = node;
            tail = node;
        }
        else
        {
            tail->next = node;
            tail = node;
        }

        _size++;

        return &node->value;
    }

    void remove(T* ptr)
    {
        ListNode<T>* prev = nullptr;
        ListNode<T>* node = head;

        while (node)
        {
            if (&node->value == ptr)
            {
                if (prev)
                    prev->next = node->next;
                else
                    head = node->next;

                if (node == tail)
                    tail = prev;

                delete node;
                _size--;

                return;
            }

            prev = node;
            node = node->next;
        }
    }

    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
};