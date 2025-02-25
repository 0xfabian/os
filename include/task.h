#pragma once

#include <arch/cpu.h>
#include <mem/heap.h>

struct Task
{
    CPU cpu;
    Task* next;

    static Task* from(void (*func)(void));
    void ready();
};

extern Task* running;
extern Task* task_list;