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
    usize size;

    void* user_stack;
    void* kernel_stack;
};

struct Task
{
    u64 krsp;
    Task* next;
    u64 tid;

    TaskState state;

    MemoryMap mm;

    FDTable fdt;

    static Task* from(void (*func)(void));
    static Task* from(const u8* data, usize size);
    static Task* dummy();
    void ready();
};

extern Task* running;
extern Task* task_list;