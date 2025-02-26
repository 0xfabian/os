#include <task.h>

Task* running;
Task* task_list;
Task* task_list_tail;

Task* Task::from(void (*func)(void))
{
    Task* task = (Task*)kmalloc(sizeof(Task));
    void* stack = kmalloc(4096);

    memset(&task->cpu, 0, sizeof(CPU));

    task->cpu.rsp = (uint64_t)stack + 4096;
    task->cpu.rbp = task->cpu.rsp;
    task->cpu.rip = (uint64_t)func;
    task->cpu.rflags = 0x202;

    kprintf("%p\n", task->cpu.rsp);

    *(uint64_t*)(task->cpu.rsp - 8) = 0x00; // ss
    *(uint64_t*)(task->cpu.rsp - 16) = task->cpu.rsp; // rsp
    *(uint64_t*)(task->cpu.rsp - 24) = task->cpu.rflags; // rflags
    *(uint64_t*)(task->cpu.rsp - 32) = 0x8; // cs
    *(uint64_t*)(task->cpu.rsp - 40) = task->cpu.rip; // rip
    *(uint64_t*)(task->cpu.rsp - 48) = 0; // error code
    *(uint64_t*)(task->cpu.rsp - 56) = task->cpu.rdi;
    *(uint64_t*)(task->cpu.rsp - 64) = task->cpu.rsi;
    *(uint64_t*)(task->cpu.rsp - 72) = task->cpu.rdx;
    *(uint64_t*)(task->cpu.rsp - 80) = task->cpu.rcx;
    *(uint64_t*)(task->cpu.rsp - 88) = task->cpu.rax;
    *(uint64_t*)(task->cpu.rsp - 96) = task->cpu.r8;
    *(uint64_t*)(task->cpu.rsp - 104) = task->cpu.r9;
    *(uint64_t*)(task->cpu.rsp - 112) = task->cpu.r10;
    *(uint64_t*)(task->cpu.rsp - 120) = task->cpu.r11;
    *(uint64_t*)(task->cpu.rsp - 128) = task->cpu.rbx;
    *(uint64_t*)(task->cpu.rsp - 136) = task->cpu.rbp;
    *(uint64_t*)(task->cpu.rsp - 144) = task->cpu.r12;
    *(uint64_t*)(task->cpu.rsp - 152) = task->cpu.r13;
    *(uint64_t*)(task->cpu.rsp - 160) = task->cpu.r14;
    *(uint64_t*)(task->cpu.rsp - 168) = task->cpu.r15;

    task->cpu.rsp -= 168;

    return task;
}

Task* Task::dummy()
{
    // this should be the first task in the queue
    // at the first switch, the kmain will store the cpu state
    // to this dummy task so basically this task is kmain

    Task* task = (Task*)kmalloc(sizeof(Task));

    return task;
}

void Task::ready()
{
    if (task_list)
    {
        task_list_tail->next = this;
        task_list_tail = this;
        task_list_tail->next = task_list;
    }
    else
    {
        task_list = this;
        task_list->next = task_list;
        task_list_tail = task_list;
    }
}