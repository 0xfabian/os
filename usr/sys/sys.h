#pragma once

#include <types.h>

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0x40
#define O_EXCL      0x80
#define O_TRUNC     0x200
#define O_APPEND    0x400
#define O_DIRECTORY 0x10000

#define IT_MASK     0xf000
#define IT_FIFO     0x1000
#define IT_CDEV     0x2000
#define IT_DIR      0x4000
#define IT_BDEV     0x6000
#define IT_REG      0x8000
#define IT_LINK     0xa000
#define IT_SOCK     0xc000

#define IP_MASK     0x01ff
#define IP_R_U      0x0100
#define IP_W_U      0x0080
#define IP_X_U      0x0040
#define IP_R_G      0x0020
#define IP_W_G      0x0010
#define IP_X_G      0x0008
#define IP_R_O      0x0004
#define IP_W_O      0x0002
#define IP_X_O      0x0001
#define IP_R        (IP_R_U | IP_R_G | IP_R_O)
#define IP_W        (IP_W_U | IP_W_G | IP_W_O)
#define IP_X        (IP_X_U | IP_X_G | IP_X_O)
#define IP_RW       (IP_R | IP_W)
#define IP_RWX      (IP_R | IP_W | IP_X)

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define FBTERM_CLEAR        1
#define FBTERM_ECHO_ON      2
#define FBTERM_ECHO_OFF     3
#define FBTERM_LB_ON        4
#define FBTERM_LB_OFF       5
#define FBTERM_GET_WIDTH    6
#define FBTERM_GET_HEIGHT   7
#define FBTERM_SET_FG_GROUP 8
#define FB_GET_WIDTH        1
#define FB_GET_HEIGHT       2

#define F_OK        0
#define X_OK        1
#define W_OK        2
#define R_OK        4

#define AT_FDCWD    -100

struct stat
{
    u64 st_dev;
    u64 st_ino;
    u64 st_nlink;
    u32 st_mode;
    u32 st_uid;
    u32 st_gid;
    int __pad0;
    u64 st_rdev;
    i64 st_size;
    i64 st_blksize;
    i64 st_blocks;
    i64 st_atime;
    u64 st_atimensec;
    i64 st_mtime;
    u64 st_mtimensec;
    i64 st_ctime;
    u64 st_ctimensec;
    i64 __reserved[3];
};

struct iovec
{
    void* iov_base;
    usize iov_len;
};

#ifdef __cplusplus
#define extern extern "C"
#endif

extern isize    read(unsigned int fd, void* buf, usize size);
extern isize    write(unsigned int fd, const void* buf, usize size);
extern int      open(const char* path, u32 flags, u32 mode);
extern int      close(unsigned int fd);
extern int      stat(const char* path, struct stat* buf);
extern int      fstat(int fd, struct stat* buf);
extern int      lstat(const char* path, struct stat* buf);
extern isize    seek(int fd, isize offset, int whence);
extern int      mprotect(void* addr, usize size, int prot);
extern u64      brk(void* addr);
extern int      signal(int signo, void (*handler)(int));
extern int      sigprocmask(int how, u32* set, u32* oldset);
extern int      ioctl(int fd, int cmd, void* arg);
extern isize    pread(int fd, void* buf, usize size, usize offset);
extern isize    pwrite(int fd, const void* buf, usize size, usize offset);
extern isize    readv(int fd, const struct iovec* iov, int iovcnt);
extern isize    writev(int fd, const struct iovec* iov, int iovcnt);
extern int      access(const char* path, int mode);
extern int      pipe(int fds[2]);
extern int      dup(int oldfd);
extern int      dup2(int oldfd, int newfd);
extern u64      gettid();
extern int      fork(void);
extern int      execve(const char* path, char* const argv[], char* const envp[]);
extern void     exit(int status);
extern int      wait(int pid, int* status, int options);
extern int      kill(int pid, int signo);
extern int      uname(void* buf);
extern int      fcntl(int fd, int op, u64 arg);
extern int      truncate(const char* path, usize size);
extern int      ftruncate(int fd, usize size);
extern int      getdents(int fd, void* buf, usize size);
extern int      getcwd(char* buf, usize size);
extern int      chdir(const char* path);
extern int      mkdir(const char* path);
extern int      rmdir(const char* path);
extern int      creat(const char* path, u32 mode);
extern int      link(const char* oldpath, const char* newpath);
extern int      unlink(const char* path);
extern int      symlink(const char* target, const char* name);
extern isize    readlink(const char* path, char* buf, usize size);
extern int      getuid(void);
extern int      getgid(void);
extern int      geteuid(void);
extern int      getegid(void);
extern int      mknod(const char* path, u32 mode, u32 dev);
extern int      arch_prctl(int op, u64* addr);
extern int      mount(const char* source, const char* target, const char* fstype);
extern int      umount(const char* target);
extern int      openat(int dirfd, const char* path, u32 flags, u32 mode);
extern int      setgroup(int group);
extern int      debug(const char* str);
