#include <task.h>

Task* running;
Task* task_list;
Task* task_list_tail;

uint64_t global_tid = 0;

Task* Task::from(void (*func)(void))
{
    Task* task = (Task*)kmalloc(sizeof(Task));
    task->tid = global_tid++;

    uint64_t stack_top = (uint64_t)kmalloc(PAGE_SIZE) + PAGE_SIZE;

    task->krsp = stack_top - sizeof(CPU);

    CPU* cpu = (CPU*)task->krsp;
    memset(cpu, 0, sizeof(CPU));

    cpu->rbp = stack_top;

    cpu->rip = (uint64_t)func;
    cpu->cs = KERNEL_CS;
    cpu->rflags = 0x202;
    cpu->rsp = stack_top;
    cpu->ss = KERNEL_DS;

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