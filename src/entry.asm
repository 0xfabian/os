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

; extern syscall_handler

; section .bss
;     entry_rax resq 1
;     entry_rsp resq 1

; section .text

; syscall_handler_asm:

;     o64 sysret

;     ; this should be called from ring 3 so because of the tss mechanism
;     ; rsp should be the kernel rsp

;     ; we should form another CPU struct on the stack in case this syscall
;     ; calls sched or something

;     mov [entry_rax], rax
;     mov [entry_rsp], rsp

;     mov rax, 0x10 ; kernel ss
;     push rax

;     push qword [entry_rsp]

;     push r11 ; rflags

;     mov rax, 0x08
;     push rax ; kernel cs

;     push rcx ; rip

;     mov rax, [entry_rax]

;     PUSH_REGS

;     ; at this point CPU struct is saved and rsp is pointing to it

;     mov rdi, rsp
;     mov rsi, rax
;     call syscall_handler

;     POP_REGS

;     iretq

;     ; mov [entry_rsp], rsp
;     ; mov rax, 0x10
;     ; push rax ; ss

;     ; push qword [entry_rsp] ; push original rsp

;     ; push r11 ; push rflags

;     ; mov rax, 0x08 
;     ; push rax ; push cs

;     ; push rcx ; push rip

;     ; iretq


; GLOBAL syscall_handler_asm