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
    SYS_STAT,
    SYS_FSTAT,
    SYS_SEEK = 8,
    SYS_MPROTECT = 10,
    SYS_BRK = 12,
    SYS_IOCTL = 16,
    SYS_PREAD,
    SYS_PWRITE,
    SYS_READV,
    SYS_WRITEV,
    SYS_ACCESS,
    SYS_PIPE,
    SYS_DUP = 32,
    SYS_DUP2,
    SYS_FORK = 57,
    SYS_EXECVE = 59,
    SYS_EXIT,
    SYS_WAIT,
    SYS_UNAME = 63,
    SYS_FCNTL = 72,
    SYS_TRUNCATE = 76,
    SYS_FTRUNCATE,
    SYS_GETDENTS,
    SYS_GETCWD,
    SYS_CHDIR,
    // SYS_RENAME = 82,
    SYS_MKDIR = 83,
    SYS_RMDIR,
    SYS_CREAT,
    SYS_LINK,
    SYS_UNLINK,
    // SYS_SYMLINK,
    SYS_READLINK = 89,
    SYS_GETUID = 102,
    SYS_GETGID = 104,
    SYS_GETEUID = 107,
    SYS_GETEGID,
    SYS_MKNOD = 133,
    SYS_ARCH_PRCTL = 158,
    SYS_MOUNT = 165,
    SYS_UMOUNT,
    SYS_EXIT_GROUP = 231,
    SYS_OPENAT = 257,
    SYS_SETGROUP = 300,
    SYS_DEBUG = 512
};

struct iovec
{
    void* iov_base;
    usize iov_len;
};

isize sys_read(unsigned int fd, void* buf, usize size);
isize sys_write(unsigned int fd, const void* buf, usize size);
int sys_open(const char* path, u32 flags, u32 mode);
int sys_close(unsigned int fd);
int sys_stat(const char* path, stat* buf);
int sys_fstat(int fd, stat* buf);
isize sys_seek(int fd, isize offset, int whence);
int sys_mprotect(void* addr, usize size, int prot);
u64 sys_brk(void* addr);
int sys_ioctl(int fd, int cmd, void* arg);
isize sys_pread(int fd, void* buf, usize size, usize offset);
isize sys_pwrite(int fd, const void* buf, usize size, usize offset);
isize sys_readv(int fd, const iovec* iov, int iovcnt);
isize sys_writev(int fd, const iovec* iov, int iovcnt);
int sys_access(const char* path, int mode);
int sys_pipe(int fds[2]);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
int sys_fork();
int sys_execve(const char* path, char* const argv[], char* const envp[]);
void sys_exit(int status);
int sys_wait(int pid, int* status, int options);
int sys_uname(void* buf);
int sys_fcntl(int fd, int op, u64 arg);
int sys_truncate(const char* path, usize size);
int sys_ftruncate(int fd, usize size);
int sys_getdents(int fd, void* buf, usize size);
int sys_getcwd(char* buf, usize size);
int sys_chdir(const char* path);
int sys_mkdir(const char* path);
int sys_rmdir(const char* path);
int sys_creat(const char* path, u32 mode);
int sys_link(const char* oldpath, const char* newpath);
int sys_unlink(const char* path);
isize sys_readlink(const char* path, char* buf, usize size);
int sys_getuid();
int sys_getgid();
int sys_geteuid();
int sys_getegid();
int sys_mknod(const char* path, u32 mode, u32 dev);
int sys_arch_prctl(int op, u64* addr);
int sys_mount(const char* source, const char* target, const char* fstype);
int sys_umount(const char* target);
int sys_openat(int dirfd, const char* path, u32 flags, u32 mode);
int sys_setgroup(int group);
int sys_debug(const char* str);
