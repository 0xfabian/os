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