#include <syscalls.h>

int do_syscall(CPU* cpu, int num)
{
    switch (num)
    {
    case SYS_READ:          return sys_read(cpu->rdi, (char*)cpu->rsi, cpu->rdx);
    case SYS_WRITE:         return sys_write(cpu->rdi, (const char*)cpu->rsi, cpu->rdx);
    case SYS_OPEN:          return sys_open((const char*)cpu->rdi, cpu->rsi);
    case SYS_CLOSE:         return sys_close(cpu->rdi);
    case SYS_IOCTL:         return sys_ioctl(cpu->rdi, cpu->rsi, (void*)cpu->rdx);
    case SYS_DUP:           return sys_dup(cpu->rdi);
    case SYS_DUP2:          return sys_dup2(cpu->rdi, cpu->rsi);
    case SYS_FORK:          return sys_fork();
    case SYS_EXECVE:        return sys_execve((const char*)cpu->rdi, (char* const*)cpu->rsi, (char* const*)cpu->rdx);
    case SYS_EXIT:          sys_exit(cpu->rdi); return 0;
    case SYS_WAIT:          return sys_wait(cpu->rdi, (int*)cpu->rsi, cpu->rdx);
    case SYS_GETDENTS:      return sys_getdents(cpu->rdi, (char*)cpu->rsi, cpu->rdx);
    case SYS_CHDIR:         return sys_chdir((const char*)cpu->rdi);
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
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->read(buf, size);
}

isize sys_write(unsigned int fd, const char* buf, usize size)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->write(buf, size);
}

int sys_open(const char* path, u32 flags)
{
    FDTable* fdt = &running->fdt;

    int fd = fdt->get_unused();

    if (fd < 0)
        return fd;

    auto file = File::open(path, flags);

    if (!file)
        return file.error();

    fdt->install(fd, file.ptr);

    return fd;
}

int sys_close(unsigned int fd)
{
    FDTable* fdt = &running->fdt;

    auto file = fdt->uninstall(fd);

    if (!file)
        return file.error();

    file->close();

    return 0;
}

int sys_ioctl(int fd, int cmd, void* arg)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->ioctl(cmd, arg);
}

int sys_dup(int oldfd)
{
    FDTable* fdt = &running->fdt;

    int newfd = fdt->get_unused();

    if (newfd < 0)
        return newfd;

    auto file = fdt->get(oldfd);

    if (!file)
        return file.error();

    fdt->install(newfd, file->dup());

    return newfd;
}

int sys_dup2(int oldfd, int newfd)
{
    FDTable* fdt = &running->fdt;

    if (!fdt->is_usable(newfd))
        return -ERR_BAD_FD;

    auto file = fdt->get(oldfd);

    if (!file)
        return file.error();

    fdt->install(newfd, file->dup());

    return newfd;
}

int sys_fork()
{
    Task* task = running->fork();

    if (!task)
        return -1;

    task->ready();

    return task->tid;
}

int sys_execve(const char* path, char* const argv[], char* const envp[])
{
    return running->execve(path, argv, envp);
}

void sys_exit(int status)
{
    running->exit(status);

    ikprintf("Task %lu should be zombie, but it's not?\n", running->tid);

    idle();
}

int sys_wait(int pid, int* status, int options)
{
    running->wait();

    return 0;
}

int sys_getdents(int fd, char* buf, usize size)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->iterate(buf, size);
}

int sys_chdir(const char* path)
{
    auto inode = Inode::get(path);

    if (!inode)
        return inode.error();

    if (!inode->is_dir())
    {
        inode->put();
        return -ERR_NOT_DIR;
    }

    running->cwd->put();
    running->cwd = inode.ptr;

    return 0;
}

void sys_debug()
{
    ikprintf("sys_debug()\n");
}