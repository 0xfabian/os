[bits 64]

extern running
extern context_switch

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

timer_handler:

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
    
    call context_switch

    mov rdi, [running]
    mov rsp, [rdi] ; restore rsp from the new task krsp

    POP_REGS

    iretq   ; restore rip, cs, rflags, rsp, ss

GLOBAL timer_handler

extern syscall_handler

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

    ; mov rdi, [running]
    ; mov [rdi], rsp

    mov rdi, rsp
    mov rsi, rax
    call syscall_handler

    ; if we return to the same task
    ; we just need to restore rcx, r11 and rsp
    ; else i think we should change rsp to the new task krsp
    ; and restore the full CPU struct and ireq

    mov rcx, [rsp + 15 * 8] ; rip
    mov r11, [rsp + 17 * 8] ; rflags

    mov rsp, [gs:20]

    o64 sysret

    ; POP_REGS

    ; iretq

GLOBAL syscall_handler_asm