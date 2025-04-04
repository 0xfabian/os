#pragma once

#include <print.h>
#include <error.h>
#include <task.h>

enum SyscallNumber
{
    SYS_READ = 0,
    SYS_WRITE,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_SEEK = 8,
    SYS_MPROTECT = 10,
    SYS_BRK = 12,
    SYS_IOCTL = 16,
    SYS_WRITEV = 20,
    SYS_DUP = 32,
    SYS_DUP2,
    SYS_FORK = 57,
    SYS_EXECVE = 59,
    SYS_EXIT,
    SYS_WAIT,
    SYS_UNAME = 63,
    SYS_FCNTL = 72,
    SYS_GETDENTS = 78,
    SYS_GETCWD,
    SYS_CHDIR,
    SYS_MKDIR = 83,
    SYS_RMDIR,
    SYS_UNLINK = 87,
    SYS_READLINK = 89,
    SYS_GETUID = 102,
    SYS_GETGID = 104,
    SYS_GETEUID = 107,
    SYS_GETEGID,
    SYS_ARCH_PRCTL = 158,
    SYS_OPENAT = 257,

    SYS_DEBUG = 512
};

struct iovec
{
    void* iov_base;
    usize iov_len;
};

isize sys_read(unsigned int fd, void* buf, usize size);
isize sys_write(unsigned int fd, const void* buf, usize size);
int sys_open(const char* path, u32 flags);
int sys_close(unsigned int fd);
isize sys_seek(int fd, isize offset, int whence);
int mprotect(void* addr, usize size, int prot);
u64 sys_brk(void* addr);
int sys_ioctl(int fd, int cmd, void* arg);
isize sys_writev(int fd, const iovec* iov, int iovcnt);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
int sys_fork();
int sys_execve(const char* path, char* const argv[], char* const envp[]);
void sys_exit(int status);
int sys_wait(int pid, int* status, int options);
int sys_uname(void* buf);
int sys_fcntl(int fd, int op, u64 arg);
int sys_getdents(int fd, void* buf, usize size);
int sys_getcwd(char* buf, usize size);
int sys_chdir(const char* path);
int sys_mkdir(const char* path);
int sys_rmdir(const char* path);
int sys_unlink(const char* path);
isize sys_readlink(const char* path, char* buf, usize size);
int sys_getuid();
int sys_getgid();
int sys_geteuid();
int sys_getegid();
int sys_arch_prctl(int op, u64* addr);
int sys_openat(int dirfd, const char* path, u32 flags);

int sys_debug(const char* str);