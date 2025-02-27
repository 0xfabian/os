#pragma once

#include <arch/cpu.h>
#include <mem/heap.h>

struct Task
{
    CPU cpu;
    Task* next;
    uint64_t tid;

    static Task* from(void (*func)(void));
    static Task* dummy();
    void kpush(uint64_t value);
    void init_stack();
    void ready();
};

extern Task* running;
extern Task* task_list;