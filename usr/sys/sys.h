#pragma once

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0x40
#define O_EXCL      0x80
#define O_TRUNC     0x200
#define O_APPEND    0x400
#define O_DIRECTORY 0x10000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FBTERM_CLEAR        1
#define FBTERM_ECHO_ON      2
#define FBTERM_ECHO_OFF     3
#define FBTERM_LB_ON        4
#define FBTERM_LB_OFF       5
#define FBTERM_GET_WIDTH    6
#define FBTERM_GET_HEIGHT   7

#ifdef __cplusplus
#define extern extern "C"
#endif

extern int open(const char* path, int flags, int mode);
extern int close(unsigned int fd);
extern long read(int fd, void* buf, unsigned long count);
extern long write(int fd, const void* buf, unsigned long count);
extern long seek(int fd, long offset, int whence);
extern int ioctl(int fd, int cmd, void* arg);
extern int dup(int oldfd);
extern int dup2(int oldfd, int newfd);
extern int fork();
extern int execve(const char* path, char* const argv[], char* const envp[]);
extern int wait4(int pid, int* status, int options, void* rusage);
extern void exit(int status);
extern int getcwd(char* buf, unsigned long size);
extern int chdir(const char* path);
extern int mkdir(const char* path, int mode);
extern int rmdir(const char* path);
extern int getdents(int fd, void* buf, unsigned long size);
extern int debug(const char* str);