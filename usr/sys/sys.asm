[bits 64]

%macro SYSCALL 2

global %1
%1:
    mov eax, %2
    syscall
    ret

%endmacro

%macro SYSCALL4 2

global %1
$%1:
    mov r10, rcx    ; rcx gets clobbered on syscall
    mov eax, %2     ; so the 4th argument is passed in r10
    syscall
    ret

%endmacro

SYSCALL     read,           0
SYSCALL     write,          1
SYSCALL     open,           2
SYSCALL     close,          3
SYSCALL     stat,           4
SYSCALL     fstat,          5
SYSCALL     seek,           8
SYSCALL     mprotect,       10
SYSCALL     brk,            12
SYSCALL     ioctl,          16
SYSCALL4    pread,          17
SYSCALL4    pwrite,         18
SYSCALL     readv,          19
SYSCALL     writev,         20
SYSCALL     access,         21
SYSCALL     pipe,           22
SYSCALL     dup,            32
SYSCALL     dup2,           33
SYSCALL     fork,           57
SYSCALL     execve,         59
SYSCALL     exit,           60
SYSCALL4    wait,           61
SYSCALL     uname,          63
SYSCALL     fcntl,          72
SYSCALL     truncate,       76
SYSCALL     ftruncate,      77
SYSCALL     getdents,       78
SYSCALL     getcwd,         79
SYSCALL     chdir,          80
SYSCALL     mkdir,          83
SYSCALL     rmdir,          84
SYSCALL     creat,          85
SYSCALL     link,           86
SYSCALL     unlink,         87
SYSCALL     readlink,       89
SYSCALL     getuid,         102
SYSCALL     getgid,         104
SYSCALL     geteuid,        107
SYSCALL     getegid,        108
SYSCALL     mknod,          133
SYSCALL     arch_prctl,     158
SYSCALL     mount,          165
SYSCALL     umount,         166
SYSCALL     openat,         257
SYSCALL     setgroup,       300
SYSCALL     debug,          512

global _start
extern main

_start:
    mov rdi, [rsp]
    lea rsi, [rsp + 8]
    call main
    mov edi, eax
    call exit
