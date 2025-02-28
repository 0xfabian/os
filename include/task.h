#pragma once

#include <arch/cpu.h>
#include <mem/heap.h>
#include <fs/fd.h>

enum TaskState
{
    TASK_BORN,
    TASK_READY
};

struct MemoryMap
{
    void* start;
    size_t size;

    void* user_stack;
    void* kernel_stack;
};

struct Task
{
    uint64_t krsp;
    Task* next;
    uint64_t tid;

    TaskState state;

    MemoryMap mm;

    FDTable fdt;

    static Task* from(void (*func)(void));
    static Task* from(const uint8_t* data, size_t size);
    static Task* dummy();
    void ready();
};

extern Task* running;
extern Task* task_list;