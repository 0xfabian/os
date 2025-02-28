#include <syscalls.h>

int syscall_handler(CPU* cpu, int num)
{
    switch (num)
    {
    case SYS_DEBUG:         sys_debug();                return 0;
    default:                                            return -ERR_NOT_IMPL;
    }
}

void sys_debug()
{
    kprintf("Debug from task %d\n", running->tid);
}