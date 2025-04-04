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
%1:
    mov r10, rcx    ; rcx gets clobbered on syscall
    mov eax, %2     ; so the 4th argument is passed in r10
    syscall
    ret

%endmacro

SYSCALL     open,       2
SYSCALL     close,      3
SYSCALL     read,       0
SYSCALL     write,      1
SYSCALL     seek,       8
SYSCALL     ioctl,      16
SYSCALL     dup,        32
SYSCALL     dup2,       33
SYSCALL     fork,       57
SYSCALL     execve,     59
SYSCALL4    wait4,      61
SYSCALL     exit,       60
SYSCALL     getcwd,     79
SYSCALL     chdir,      80
SYSCALL     mkdir,      83
SYSCALL     rmdir,      84
SYSCALL     getdents,   78
SYSCALL     debug,      512

global _start
extern main

_start:
    mov rdi, [rsp]
    lea rsi, [rsp + 8]
    call main
    mov edi, eax
    call exit