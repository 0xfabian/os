[bits 64]

extern timer_handler
extern syscall_handler
extern breakpoint_handler
extern running
extern schedule
extern leave_and_sched

global timer_handler_asm
global syscall_handler_asm
global breakpoint_handler_asm
global yield
global pause
global switch_now

%macro PUSH_REGS 0

    push rdi    ; save general purpose registers
    push rsi
    push rdx
    push rcx
    push rax
    push r8
    push r9
    push r10
    push r11

    push rbx    ; save callee-saved registers
    push rbp
    push r12
    push r13
    push r14
    push r15

%endmacro

%macro POP_REGS 0

    pop r15   ; restore callee-saved registers
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    pop r11   ; restore general purpose registers
    pop r10
    pop r9
    pop r8
    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi

%endmacro

timer_handler_asm:

    ; at this point rsp should be the kernel rsp
    ; if this is a kernel thread rsp is already kernel rsp
    ; if it's a user process, this means that we transitioned from ring 3 to ring 0
    ; so rsp gets loaded with the rsp0 from the tss struct

    ; right now the stack looks like this
    ; ss
    ; rsp
    ; rflags
    ; cs
    ; rip <- rsp

    PUSH_REGS

    ; CPU struct is saved on the kernel stack now

    mov rdi, [running]
    mov [rdi], rsp ; save rsp in the current task krsp

    call timer_handler

    mov rdi, [running]
    mov rsp, [rdi] ; restore rsp from the new task krsp

    POP_REGS

    iretq   ; restore rip, cs, rflags, rsp, ss

yield:

    ; !!! the context is saved on top of the current stack
    ; since this is called from C++ code
    ; we can use rdi, rsi and rax as temporary registers to save the cpu context

    pushfq
    pop rdi         ; save the flags pre cli

    cli             ; this is a critical section

    pop rax         ; save rip
    mov rsi, rsp

    ; use kernel cs and ss since this should only be called from kernel
    sub rsp, 8
    mov qword [rsp], 0x10   ; push ss

    push rsi                ; push rsp
    push rdi                ; push rflags

    sub rsp, 8
    mov qword [rsp], 0x08   ; push cs

    push rax                ; push rip

    PUSH_REGS

    mov rdi, [running]
    mov [rdi], rsp

    call schedule

pause:

    ; this function is nearly identical to yield
    ; we just call leave_and_sched instead of schedule

    ; !!! the context is saved on top of the current stack
    ; since this is called from C++ code
    ; we can use rdi, rsi and rax as temporary registers to save the cpu context

    pushfq
    pop rdi         ; save the flags pre cli

    cli             ; this is a critical section

    pop rax         ; save rip
    mov rsi, rsp

    ; use kernel cs and ss since this should only be called from kernel
    sub rsp, 8
    mov qword [rsp], 0x10   ; push ss

    push rsi                ; push rsp
    push rdi                ; push rflags

    sub rsp, 8
    mov qword [rsp], 0x08   ; push cs

    push rax                ; push rip

    PUSH_REGS

    mov rdi, [running]
    mov [rdi], rsp

    call leave_and_sched

syscall_handler_asm:

    ; even though we switched from ring 3 to ring 0
    ; rsp is still the user rsp
    ; so we need to swap it with the kernel rsp located in tss.rsp0

    ; we should also form another CPU struct on the stack in case this syscall
    ; calls sched or something

    ; use tss.rsp2 for storage
    mov [gs:20], rsp
    mov rsp, [gs:4] ; change to kernel stack tss.rsp0

    sub rsp, 8
    mov qword [rsp], 0x1b ; push ss

    push qword [gs:20] ; push user rsp

    push r11 ; push rflags

    sub rsp, 8
    mov qword [rsp], 0x23 ; push cs

    push rcx ; push rip

    PUSH_REGS

    ; at this point CPU struct is saved and rsp is pointing to it

    mov rdi, [running] ; save krsp
    mov [rdi], rsp

    mov rdi, rsp
    mov rsi, rax
    call syscall_handler

    POP_REGS

    ; we can't use gs:20 here because if this syscall blocks,
    ; another task might call a syscall and overwrite gs:20
    ; so instead we restore the user rsp from the kernel stack
    ; !!! if the syscall paused, the task krsp is not where the syscall left it, this might be a problem
    mov rsp, [rsp + 24]

    o64 sysret

    ; sometimes maybe we need to reschedule
    ; so we should change to the new task krsp
    ; and do iretq

switch_now:

    mov rdi, [running]
    mov rsp, [rdi] ; move the kernel rsp where the CPU is stored

    POP_REGS

    iretq

breakpoint_handler_asm:

    PUSH_REGS

    mov rdi, [running]
    mov [rdi], rsp

    call breakpoint_handler
