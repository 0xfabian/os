#include <syscalls.h>

u64 do_syscall(CPU* cpu, int num)
{
    switch (num)
    {
    case SYS_READ:          return sys_read(cpu->rdi, (char*)cpu->rsi, cpu->rdx);
    case SYS_WRITE:         return sys_write(cpu->rdi, (const char*)cpu->rsi, cpu->rdx);
    case SYS_OPEN:          return sys_open((const char*)cpu->rdi, cpu->rsi);
    case SYS_CLOSE:         return sys_close(cpu->rdi);
    case SYS_SEEK:          return sys_seek(cpu->rdi, cpu->rsi, cpu->rdx);
    case SYS_MPROTECT:      return mprotect((void*)cpu->rdi, cpu->rsi, cpu->rdx);
    case SYS_BRK:           return sys_brk((void*)cpu->rdi);
    case SYS_IOCTL:         return sys_ioctl(cpu->rdi, cpu->rsi, (void*)cpu->rdx);
    case SYS_WRITEV:        return sys_writev(cpu->rdi, (const iovec*)cpu->rsi, cpu->rdx);
    case SYS_DUP:           return sys_dup(cpu->rdi);
    case SYS_DUP2:          return sys_dup2(cpu->rdi, cpu->rsi);
    case SYS_FORK:          return sys_fork();
    case SYS_EXECVE:        return sys_execve((const char*)cpu->rdi, (char* const*)cpu->rsi, (char* const*)cpu->rdx);
    case SYS_EXIT:          sys_exit(cpu->rdi); return 0;
    case SYS_WAIT:          return sys_wait(cpu->rdi, (int*)cpu->rsi, cpu->rdx);
    case SYS_UNAME:         return sys_uname((void*)cpu->rdi);
    case SYS_FCNTL:         return sys_fcntl(cpu->rdi, cpu->rsi, cpu->rdx);
    case SYS_GETDENTS:      return sys_getdents(cpu->rdi, (char*)cpu->rsi, cpu->rdx);
    case SYS_GETCWD:        return sys_getcwd((char*)cpu->rdi, cpu->rsi);
    case SYS_CHDIR:         return sys_chdir((const char*)cpu->rdi);
    case SYS_UNLINK:        return sys_unlink((const char*)cpu->rdi);
    case SYS_READLINK:      return sys_readlink((const char*)cpu->rdi, (char*)cpu->rsi, cpu->rdx);
    case SYS_GETUID:        return sys_getuid();
    case SYS_GETGID:        return sys_getgid();
    case SYS_GETEUID:       return sys_geteuid();
    case SYS_GETEGID:       return sys_getegid();
    case SYS_ARCH_PRCTL:    return sys_arch_prctl(cpu->rdi, (u64*)cpu->rsi);
    case SYS_OPENAT:        return sys_openat(cpu->rdi, (const char*)cpu->rsi, cpu->rdx);
    case SYS_DEBUG:         return sys_debug((const char*)cpu->rdi);
    default:                return -ERR_NOT_IMPL;
    }
}

extern "C" void syscall_handler(CPU* cpu, int num)
{
    cpu->rax = do_syscall(cpu, num);

    // if ((ErrorCode)cpu->rax == -ERR_NOT_IMPL)
    //     kprintf(WARN "syscall %d not implemented\n", num);
    // else
    //     kprintf(INFO "syscall %d returned %lx\n", num, cpu->rax);
}

isize sys_read(unsigned int fd, void* buf, usize size)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->read(buf, size);
}

isize sys_write(unsigned int fd, const void* buf, usize size)
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

isize sys_seek(int fd, isize offset, int whence)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->seek(offset, whence);
}

int mprotect(void* addr, usize size, int prot)
{
    return 0;
}

u64 sys_brk(void* addr)
{
    u64 min_brk = running->mm->start_brk;
    u64 org_brk = running->mm->brk;
    u64 new_brk = (u64)addr;

    if (new_brk < min_brk)
        return org_brk;

    u64 org_brk_page = PAGE_ALIGN(org_brk);
    u64 new_brk_page = PAGE_ALIGN(new_brk);

    if (new_brk_page > org_brk_page)
    {
        usize pages = PAGE_COUNT(new_brk_page - org_brk_page);

        vmm.alloc_pages(running->mm->pml4, org_brk_page, pages, PE_WRITE | PE_USER);
    }
    else if (new_brk_page < org_brk_page)
    {
        // free pages
    }

    running->mm->brk = new_brk;

    return new_brk;
}

int sys_ioctl(int fd, int cmd, void* arg)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->ioctl(cmd, arg);
}

isize sys_writev(int fd, const iovec* iov, int iovcnt)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    isize total = 0;

    for (int i = 0; i < iovcnt; i++)
    {
        isize ret = file->write(iov[i].iov_base, iov[i].iov_len);

        if (ret < 0)
            return ret;

        total += ret;
    }

    return total;
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

    kprintf("Task %lu should be zombie, but it's not?\n", running->tid);

    idle();
}

int sys_wait(int pid, int* status, int options)
{
    running->wait();

    return 0;
}

int sys_uname(void* buf)
{
    struct utsname
    {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
    };

    auto uname = (utsname*)buf;

    strcpy(uname->sysname, "Linux");
    strcpy(uname->nodename, "localhost");
    strcpy(uname->release, "5.12.167");
    strcpy(uname->version, "5.12.167");
    strcpy(uname->machine, "x86_64");

    return 0;
}

int sys_fcntl(int fd, int op, u64 arg)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    // F_GETFL
    if (op == 3)
        return file->flags;

    return -1;
}

int sys_getdents(int fd, void* buf, usize size)
{
    auto file = running->fdt.get(fd);

    if (!file)
        return file.error();

    return file->iterate(buf, size);
}

int sys_getcwd(char* buf, usize size)
{
    usize len = strlen(running->cwd_str);

    if (len >= size)
        return -ERR_NO_SPACE;

    memcpy(buf, running->cwd_str, len + 1);

    return len;
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

    kfree(running->cwd_str);

    char* new_cwd_str;

    if (*path == '/')
        new_cwd_str = strdup(path);
    else
    {
        new_cwd_str = (char*)kmalloc(strlen(running->cwd_str) + strlen(path) + 2);

        strcpy(new_cwd_str, running->cwd_str);
        strcat(new_cwd_str, "/");
        strcat(new_cwd_str, path);
    }

    // this frees new_cwd_str and allocates a new one
    running->cwd_str = path_normalize(new_cwd_str);

    running->cwd->put();
    running->cwd = inode.ptr;

    return 0;
}

int sys_unlink(const char* path)
{
    auto inode = Inode::get(path);

    if (!inode)
        return inode.error();

    if (inode->is_dir())
    {
        inode->put();
        return -ERR_IS_DIR;
    }

    inode->put();

    // can this fail?
    auto dir = Inode::get(dirname(path));

    int ret = dir->unlink(basename(path));

    dir->put();

    return ret;
}

isize sys_readlink(const char* path, char* buf, usize size)
{
    return -ERR_NOT_FOUND;
}

int sys_getuid()
{
    return 0;
}

int sys_getgid()
{
    return 0;
}

int sys_geteuid()
{
    return 0;
}

int sys_getegid()
{
    return 0;
}

int sys_arch_prctl(int op, u64* addr)
{
    // ARCH_SET_FS
    if (op == 0x1002)
    {
        // IA32_FS_BASE
        wrmsr(0xc0000100, (u64)addr);
        return 0;
    }

    return -1;
}

#define AT_FDCWD    (-100)

int sys_openat(int dirfd, const char* path, u32 flags)
{
    if (dirfd != AT_FDCWD)
        return -ERR_BAD_FD;

    return sys_open(path, flags);
}

int sys_debug(const char* str)
{
    if (strcmp(str, "inode_table") == 0)
        inode_table.debug();
    else if (strcmp(str, "file_table") == 0)
        file_table.debug();
    else if (strcmp(str, "heap") == 0)
        heap.debug();

    return 0;
}