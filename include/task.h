#pragma once

#include <arch/cpu.h>
#include <mem/heap.h>

struct Task
{
    uint64_t krsp;
    Task* next;
    uint64_t tid;

    static Task* from(void (*func)(void));
    static Task* dummy();
    void ready();
};

extern Task* running;
extern Task* task_list;