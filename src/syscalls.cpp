#include <syscalls.h>

int do_syscall(CPU* cpu, int num)
{
    switch (num)
    {
    case SYS_DEBUG:         sys_debug();                return 0;
    default:                                            return -ERR_NOT_IMPL;
    }
}

extern "C" void syscall_handler(CPU* cpu, int num)
{
    cpu->rax = do_syscall(cpu, num);
}

void sys_debug()
{
    ikprintf("sys_debug()\n");
}