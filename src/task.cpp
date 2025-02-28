#include <task.h>

#define KERNEL_STACK_SIZE   PAGE_SIZE
#define USER_STACK_SIZE     PAGE_SIZE

Task* running;
Task* task_list;
Task* task_list_tail;

u64 global_tid = 0;

Task* Task::from(void (*func)(void))
{
    Task* task = (Task*)kmalloc(sizeof(Task));

    task->next = nullptr;
    task->tid = global_tid++;
    task->state = TASK_BORN;

    task->mm.start = nullptr;
    task->mm.size = 0;
    task->mm.user_stack = nullptr;
    task->mm.kernel_stack = kmalloc(KERNEL_STACK_SIZE);

    u64 kstack_top = (u64)task->mm.kernel_stack + KERNEL_STACK_SIZE;

    task->krsp = kstack_top - sizeof(CPU);

    CPU* cpu = (CPU*)task->krsp;
    memset(cpu, 0, sizeof(CPU));

    cpu->rbp = kstack_top;

    cpu->rip = (u64)func;
    cpu->cs = KERNEL_CS;
    cpu->rflags = 0x202;
    cpu->rsp = kstack_top;
    cpu->ss = KERNEL_DS;

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        task->fdt.files[i] = nullptr;

    return task;
}

Task* Task::from(const u8* data, usize size)
{
    Task* task = (Task*)kmalloc(sizeof(Task));

    task->next = nullptr;
    task->tid = global_tid++;
    task->state = TASK_BORN;

    task->mm.start = kmalloc(size);
    task->mm.size = size;
    task->mm.user_stack = kmalloc(USER_STACK_SIZE);
    task->mm.kernel_stack = kmalloc(KERNEL_STACK_SIZE);

    memcpy(task->mm.start, data, size);

    u64 ustack_top = (u64)task->mm.user_stack + USER_STACK_SIZE;
    u64 kstack_top = (u64)task->mm.kernel_stack + KERNEL_STACK_SIZE;

    task->krsp = kstack_top - sizeof(CPU);

    CPU* cpu = (CPU*)task->krsp;
    memset(cpu, 0, sizeof(CPU));

    cpu->rbp = kstack_top;

    cpu->rip = (u64)task->mm.start;
    cpu->cs = USER_CS;
    cpu->rflags = 0x202;
    cpu->rsp = kstack_top;
    cpu->ss = USER_DS;

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        task->fdt.files[i] = nullptr;

    return task;
}

Task* Task::dummy()
{
    // this should be the first task in the queue
    // at the first switch, the kmain will store the cpu state
    // to this dummy task so basically this task is kmain

    Task* task = (Task*)kmalloc(sizeof(Task));

    task->next = nullptr;
    task->tid = global_tid++;
    task->state = TASK_BORN;

    task->mm.start = nullptr;
    task->mm.size = 0;
    task->mm.user_stack = nullptr;
    task->mm.kernel_stack = nullptr;

    // dont need to initialize krsp or the whole CPU struct

    for (int i = 0; i < FD_TABLE_SIZE; i++)
        task->fdt.files[i] = nullptr;

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

    state = TASK_READY;
}