#pragma once

#include <arch/gdt.h>
#include <arch/pic.h>
#include <arch/pit.h>
#include <mem/heap.h>
#include <fs/mount.h>
#include <fs/fd.h>
#include <elf.h>
#include <waitq.h>

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

enum Signal
{
    SIG_STOP = 1,
    SIG_KILL
};

enum SigDefaultAction
{
    ACTION_IGNORE,
    ACTION_STOP,
    ACTION_CONTINUE,
    ACTION_TERM
};

using SigHandler = void (*)(int);

#define SIG_DFL ((SigHandler)0)
#define SIG_IGN ((SigHandler)1)

#define SIG_BLOCK       0
#define SIG_UNBLOCK     1
#define SIG_SETMASK     2

struct SignalState
{
    u32 pending;
    u32 blocked;
    SigHandler handlers[32];

    void init();
    void init(SignalState* other);

    bool can_ignore(Signal sig);
    void set_pending(Signal sig);
    bool should_deliver(Signal sig);

    int set_handler(Signal sig, SigHandler handler);
    int sigprocmask(int how, u32* set, u32* oldset);
};

struct Task
{
    u64 krsp;

    // used for the hierarchy
    Task* parent;
    Task* children;
    Task* next_sibling;

    // pointer in the global task list
    Task* prev_global;
    Task* next_global;

    u64 tid;
    char name[32];
    int group;

    TaskState state;
    MemoryMap* mm;
    char* cwd_str;
    Inode* cwd;
    FDTable fdt;
    WaitQueue* waitq;

    SignalState sig_state;

    int exit_code;

    // pointers in the ready queue, only relevant if in ready state
    Task* prev;
    Task* next;

    static Task* from(void (*func)(void), const char* name);
    static Task* from(const char* path);
    static Task* find(u64 tid);
    static Task* dummy();

    Task* fork();
    int execve(const char* path, char* const argv[], char* const envp[]);

    void add_child(Task* child);
    void remove_child(Task* child);
    Task* find_zombie_child();

    void enter_rq();
    void leave_rq();
    void ready();
    void exit(int code);
    int wait(int* status);
    int kill(Signal sig);

    static void debug();
};

extern Task* running;

int exit_group(int group);
int kill_group(int group, Signal sig);

void sched_init();
extern "C" void schedule();
extern "C" void leave_and_sched();
extern "C" void yield();
extern "C" void pause();
