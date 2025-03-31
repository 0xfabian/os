#pragma once

#include <arch/gdt.h>
#include <arch/pic.h>
#include <arch/pit.h>
#include <mem/heap.h>
#include <fs/mount.h>
#include <fs/fd.h>
#include <elf.h>

#define KERNEL_STACK_PAGES      2   // 8KB
#define KERNEL_STACK_SIZE       (KERNEL_STACK_PAGES * PAGE_SIZE)

#define USER_STACK_PAGES        16  // 64KB
#define USER_STACK_SIZE         (USER_STACK_PAGES * PAGE_SIZE)
#define USER_STACK_TOP          0x800000000000
#define USER_STACK_BOTTOM       (USER_STACK_TOP - USER_STACK_SIZE)

enum TaskState
{
    TASK_BORN,
    TASK_READY,
    TASK_SLEEPING,
    TASK_ZOMBIE,
    TASK_DEAD,
    TASK_IDLE
};

struct MemoryMap
{
    PML4* pml4;

    u64 start_brk;
    u64 brk;
    void* user_stack;
    void* kernel_stack;
};

struct Task
{
    u64 krsp;
    Task* parent;
    u32 active_children;
    u64 tid;

    TaskState state;
    MemoryMap* mm;
    char* cwd_str;
    Inode* cwd;
    FDTable fdt;

    int exit_code;

    // only relevant if in ready state
    Task* prev;
    Task* next;

    static Task* from(void (*func)(void));
    static Task* from(const char* path);
    static Task* dummy();

    Task* fork();
    int execve(const char* path, char* const argv[], char* const envp[]);

    void enter_rq();
    void leave_rq();
    void ready();
    void sleep();
    void exit(int code);
    void wait();
    void return_from_syscall(int ret);
};

extern Task* running;

void sched_init();
extern "C" void schedule();
extern "C" void leave_and_sched();
extern "C" void yield();
extern "C" void pause();