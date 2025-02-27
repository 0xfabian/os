#include <task.h>

Task* running;
Task* task_list;
Task* task_list_tail;

uint64_t global_tid = 0;

Task* Task::from(void (*func)(void))
{
    Task* task = (Task*)kmalloc(sizeof(Task));

    task->tid = global_tid++;

    memset(&task->cpu, 0, sizeof(CPU));

    task->cpu.rsp = (uint64_t)kmalloc(PAGE_SIZE) + PAGE_SIZE;
    task->cpu.rbp = task->cpu.rsp;
    task->cpu.rip = (uint64_t)func;
    task->cpu.rflags = 0x202;

    task->init_stack();

    return task;
}

Task* Task::dummy()
{
    // this should be the first task in the queue
    // at the first switch, the kmain will store the cpu state
    // to this dummy task so basically this task is kmain

    Task* task = (Task*)kmalloc(sizeof(Task));

    task->tid = global_tid++;

    return task;
}

void Task::kpush(uint64_t value)
{
    cpu.rsp -= 8;
    *(uint64_t*)cpu.rsp = value;
}

void Task::init_stack()
{
    // interrupt frame
    kpush(0x13); // ss
    kpush(cpu.rsp);
    kpush(cpu.rflags);
    kpush(0x08); // cs
    kpush(cpu.rip);

    // error code
    kpush(0);

    // general purpose registers
    kpush(cpu.rdi);
    kpush(cpu.rsi);
    kpush(cpu.rdx);
    kpush(cpu.rcx);
    kpush(cpu.rax);
    kpush(cpu.r8);
    kpush(cpu.r9);
    kpush(cpu.r10);
    kpush(cpu.r11);

    // callee-saved registers
    kpush(cpu.rbx);
    kpush(cpu.rbp);
    kpush(cpu.r12);
    kpush(cpu.r13);
    kpush(cpu.r14);
    kpush(cpu.r15);
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