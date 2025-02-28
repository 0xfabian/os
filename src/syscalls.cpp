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
    // ikprintf("stack: %lx         kstack: %lx\n", running->mm.user_stack, running->mm.kernel_stack);
    // ikprintf("entry rsp: %lx\n", cpu->rsp);

    // u64 rsp;
    // asm volatile ("mov %%rsp, %0" : "=r"(rsp));
    // ikprintf("current rsp: %lx\n", rsp);

    // idle();

    cpu->rax = do_syscall(cpu, num);
}

void sys_debug()
{
    // kprintf right now would enable sti at the end
    // which is no good because we are in a syscall

    fbterm.write("sys_debug()\n", 12);
}