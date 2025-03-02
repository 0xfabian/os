#pragma once

#include <arch/cpu.h>
#include <mem/heap.h>
#include <fs/fd.h>

#define KERNEL_STACK_SIZE   PAGE_SIZE
#define USER_STACK_SIZE     PAGE_SIZE

enum TaskState
{
    TASK_BORN,
    TASK_READY,
    TASK_RUNNING,
    TASK_WAITING,
    TASK_ZOMBIE,
    TASK_DEAD
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

    MemoryMap* mm;

    FDTable fdt;

    static Task* from(void (*func)(void));
    static Task* from(const u8* data, usize size);
    static Task* dummy();
    void ready();
};

extern Task* running;
extern Task* task_list;