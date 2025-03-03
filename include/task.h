#pragma once

#include <arch/gdt.h>
#include <arch/pic.h>
#include <mem/heap.h>
#include <fs/fd.h>

#define KERNEL_STACK_PAGES      2   // 8KB
#define KERNEL_STACK_SIZE       (KERNEL_STACK_PAGES * PAGE_SIZE)

#define USER_STACK_PAGES        256 // 1MB
#define USER_STACK_SIZE         (USER_STACK_PAGES * PAGE_SIZE)
#define USER_STACK_TOP          0x800000000000
#define USER_STACK_BOTTOM       (USER_STACK_TOP - USER_STACK_SIZE)

enum TaskState
{
    TASK_BORN,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_ZOMBIE,
    TASK_DEAD
};

struct MemoryMap
{
    PML4* pml4;

    void* start;
    usize size;

    void* user_stack;
    void* kernel_stack;
};

struct Task
{
    u64 krsp;
    Task* next;
    Task* parent;
    u64 tid;

    TaskState state;
    MemoryMap* mm;
    FDTable fdt;

    int exit_code;

    static Task* from(void (*func)(void));
    static Task* from(const u8* data, usize size);
    static Task* dummy();

    Task* fork();
    int execve(const char* path, char* const argv[], char* const envp[]);
    void exit(int code);
    void ready();
    void sleep();
    void return_from_syscall(int ret);
};

extern Task* running;
extern Task* task_list;

void sched_init();
Task* get_next_task();
void schedule();