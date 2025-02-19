// #pragma once

// #include <mem/heap.h>

// template <typename T>
// struct TableEntry
// {
//     T value;
//     TableEntry* next;
// };

// template <typename T>
// struct Table
// {
//     struct Iterator
//     {
//         TableEntry<T>* entry;

//         Iterator(TableEntry<T>* _entry) : entry(_entry) {}

//         T& operator*() { return entry->value; }
//         Iterator& operator++() { entry = entry->next; return *this; }
//         bool operator!=(const Iterator& other) { return entry != other.entry; }
//     };

//     TableEntry<T>* head = nullptr;
//     TableEntry<T>* tail = nullptr;
//     size_t _size = 0;

//     bool empty() { return _size == 0; };
//     size_t size() { return _size; };

//     T* alloc()
//     {
//         TableEntry<T>* entry = new TableEntry<T>();

//         if (!head)
//         {
//             head = entry;
//             tail = entry;
//         }
//         else
//         {
//             tail->next = entry;
//             tail = entry;
//         }

//         _size++;

//         return &entry->value;
//     }

//     void free(T* ptr)
//     {
//         TableEntry<T>* prev = nullptr;
//         TableEntry<T>* entry = head;

//         while (entry)
//         {
//             if (&entry->value == ptr)
//             {
//                 if (prev)
//                     prev->next = entry->next;
//                 else
//                     head = entry->next;

//                 if (entry == tail)
//                     tail = prev;

//                 _size--;
//                 delete entry;
//                 return;
//             }

//             prev = entry;
//             entry = entry->next;
//         }
//     }

//     Iterator begin() { return Iterator(head); }
//     Iterator end() { return Iterator(nullptr); }
// };