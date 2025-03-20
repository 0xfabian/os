[bits 64]

global open
global close
global write
global seek
global ioctl
global read
global dup
global dup2
global fork
global execve
global wait4
global exit
global getcwd
global chdir
global getdents
global debug

open:
    mov eax, 2
    syscall
    ret

close:
    mov eax, 3
    syscall
    ret

read:
    mov eax, 0
    syscall
    ret

write:
    mov eax, 1
    syscall
    ret

seek:
    mov eax, 8
    syscall
    ret

ioctl:
    mov eax, 16
    syscall
    ret

dup:
    mov eax, 32
    syscall
    ret

dup2:
    mov eax, 33
    syscall
    ret

fork:
    mov eax, 57
    syscall
    ret

execve:
    mov eax, 59
    syscall
    ret

wait4:
    mov r10, rcx ; rcx gets clobbered on syscall
    mov eax, 61  ; so the 4th argument is passed in r10
    syscall
    ret

exit:
    mov eax, 60
    syscall
    ret

getcwd:
    mov eax, 79
    syscall
    ret

chdir:
    mov eax, 80
    syscall
    ret

getdents:
    mov eax, 78
    syscall
    ret

debug:
    mov eax, 512
    syscall
    ret

global _start
extern main

_start:
    mov rdi, [rsp]
    lea rsi, [rsp + 8]
    call main
    mov edi, eax
    call exit