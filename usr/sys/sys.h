#pragma once

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2
#define O_CREAT     0x40
#define O_EXCL      0x80
#define O_TRUNC     0x200
#define O_APPEND    0x400

#define FBTERM_CLEAR        1
#define FBTERM_ECHO_ON      2
#define FBTERM_ECHO_OFF     3
#define FBTERM_LB_ON        4
#define FBTERM_LB_OFF       5
#define FBTERM_GET_WIDTH    6
#define FBTERM_GET_HEIGHT   7

extern "C" int open(const char* path, int flags, int mode);
extern "C" int close(unsigned int fd);
extern "C" long read(int fd, void* buf, unsigned long count);
extern "C" long write(int fd, const void* buf, unsigned long count);
extern "C" long seek(int fd, long offset, int whence);
extern "C" int ioctl(int fd, int cmd, void* arg);
extern "C" int dup(int oldfd);
extern "C" int dup2(int oldfd, int newfd);
extern "C" int fork();
extern "C" int execve(const char* path, char* const argv[], char* const envp[]);
extern "C" int wait4(int pid, int* status, int options, void* rusage);
extern "C" void exit(int status);
extern "C" int getcwd(char* buf, unsigned long size);
extern "C" int chdir(const char* path);
extern "C" int getdents(int fd, char* buf, unsigned long size);
extern "C" int debug(const char* str);