#include <syscalls.h>

int do_syscall(CPU* cpu, int num)
{
    switch (num)
    {
    case SYS_READ:          return sys_read(cpu->rdi, (char*)cpu->rsi, cpu->rdx);
    case SYS_WRITE:         return sys_write(cpu->rdi, (const char*)cpu->rsi, cpu->rdx);
    case SYS_OPEN:          return sys_open((const char*)cpu->rdi, cpu->rsi);
    case SYS_CLOSE:         return sys_close(cpu->rdi);
    case SYS_FORK:          return sys_fork();
    case SYS_EXECVE:        return sys_execve((const char*)cpu->rdi, (char* const*)cpu->rsi, (char* const*)cpu->rdx);
    case SYS_EXIT:          sys_exit(cpu->rdi); return 0;
    case SYS_WAIT:          return sys_wait(cpu->rdi, (int*)cpu->rsi, cpu->rdx);
    case SYS_DEBUG:         sys_debug(); return 0;
    default:                return -ERR_NOT_IMPL;
    }
}

extern "C" void syscall_handler(CPU* cpu, int num)
{
    cpu->rax = do_syscall(cpu, num);
}

isize sys_read(unsigned int fd, char* buf, usize size)
{
    // ikprintf("sys_read(%d, %p, %lu)\n", fd, buf, size);

    isize ret = fbterm.read(buf, size);

    // this could never return here

    return ret;
}

isize sys_write(unsigned int fd, const char* buf, usize size)
{
    // ikprintf("sys_write(%d, %p, %lu)\n", fd, buf, size);

    fbterm.write(buf, size);

    return -1;
}

int sys_open(const char* path, u32 flags)
{
    ikprintf("sys_open(%s, %x)\n", path, flags);

    return -1;
}

int sys_close(unsigned int fd)
{
    ikprintf("sys_close(%d)\n", fd);

    return -1;
}

int sys_fork()
{
    ikprintf("sys_fork()\n");

    return 1;
}

int sys_execve(const char* path, char* const argv[], char* const envp[])
{
    ikprintf("sys_execve(%s, %p, %p)\n", path, argv, envp);

    ikprintf("argv = { ");

    int i = 0;
    while (argv[i])
    {
        ikprintf("\"%s\", ", argv[i]);
        i++;
    }

    ikprintf("nullptr }\n");

    return -1;
}

void sys_exit(int status)
{
    ikprintf("sys_exit(%d)\n", status);

    idle();
}

int sys_wait(int pid, int* status, int options)
{
    ikprintf("sys_wait(%d, %p, %d)\n", pid, status, options);

    return -1;
}

void sys_debug()
{
    ikprintf("sys_debug()\n");
}