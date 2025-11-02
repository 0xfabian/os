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
    SYS_LSTAT,
    SYS_SEEK = 8,
    SYS_MPROTECT = 10,      // dummy implementation
    SYS_BRK = 12,           // doesn't free pages
    SYS_SIGNAL,             // implementing ...
    SYS_SIGPROCMASK,        // implementing ...
    SYS_SIGRETURN,          // implementing ...
    SYS_IOCTL,
    SYS_PREAD,
    SYS_PWRITE,
    SYS_READV,
    SYS_WRITEV,
    SYS_ACCESS,
    SYS_PIPE,
    SYS_YIELD = 24,         // not implemented
    SYS_DUP = 32,
    SYS_DUP2,
    SYS_PAUSE,              // not implemented
    SYS_NANOSLEEP,          // not implemented
    SYS_ALARM = 37,         // not implemented
    SYS_GETTID = 39,
    SYS_FORK = 57,
    SYS_EXECVE = 59,
    SYS_EXIT,
    SYS_WAIT,
    SYS_KILL,               // implementing ...
    SYS_UNAME,              // dummy implementation
    SYS_FCNTL = 72,         // partial implementation
    SYS_TRUNCATE = 76,
    SYS_FTRUNCATE,
    SYS_GETDENTS,
    SYS_GETCWD,
    SYS_CHDIR,
    SYS_RENAME = 82,        // not implemented
    SYS_MKDIR,
    SYS_RMDIR,
    SYS_CREAT,
    SYS_LINK,
    SYS_UNLINK,
    SYS_SYMLINK,
    SYS_READLINK,
    SYS_CHMOD,              // not implemented
    SYS_FCHMOD,             // not implemented
    SYS_GETTIMEOFDAY = 96,  // not implemented
    SYS_GETUID = 102,       // dummy implementation
    SYS_GETGID = 104,       // dummy implementation
    SYS_GETEUID = 107,      // dummy implementation
    SYS_GETEGID,            // dummy implementation
    SYS_MKNOD = 133,
    SYS_ARCH_PRCTL = 158,   // partial implementation
    SYS_MOUNT = 165,        // not implemented
    SYS_UMOUNT,             // not implemented
    SYS_EXIT_GROUP = 231,   // same as SYS_EXIT
    SYS_OPENAT = 257,       // doesn't respect dirfd
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
int sys_lstat(const char* path, stat* buf);
isize sys_seek(int fd, isize offset, int whence);
int sys_mprotect(void* addr, usize size, int prot);
u64 sys_brk(void* addr);
int sys_signal(int signo, SigHandler handler);
int sys_sigprocmask(int how, u32* set, u32* oldset);
int sys_sigreturn();
int sys_ioctl(int fd, int cmd, void* arg);
isize sys_pread(int fd, void* buf, usize size, usize offset);
isize sys_pwrite(int fd, const void* buf, usize size, usize offset);
isize sys_readv(int fd, const iovec* iov, int iovcnt);
isize sys_writev(int fd, const iovec* iov, int iovcnt);
int sys_access(const char* path, int mode);
int sys_pipe(int fds[2]);
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);
u64 sys_gettid();
int sys_fork();
int sys_execve(const char* path, char* const argv[], char* const envp[]);
void sys_exit(int status);
int sys_wait(int pid, int* status, int options);
int sys_kill(int pid, int signo);
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
int sys_symlink(const char* target, const char* name);
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
