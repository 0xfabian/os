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
    SYS_IOCTL = 16,
    SYS_DUP = 32,
    SYS_DUP2,
    SYS_FORK = 57,
    SYS_EXECVE = 59,
    SYS_EXIT,
    SYS_WAIT,
    SYS_GETDENTS = 78,
    SYS_CHDIR = 80,

    SYS_DEBUG = 512
};

isize sys_read(unsigned int fd, char* buf, usize size);
isize sys_write(unsigned int fd, const char* buf, usize size);
int sys_open(const char* path, u32 flags);
int sys_close(unsigned int fd);
int sys_ioctl(int fd, int cmd, void* arg);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
int sys_fork();
int sys_execve(const char* path, char* const argv[], char* const envp[]);
void sys_exit(int status);
int sys_wait(int pid, int* status, int options);
int sys_getdents(int fd, char* buf, usize size);
int sys_chdir(const char* path);

void sys_debug();