[bits 64]

extern running
extern context_switch

timer_handler:

    push rax    ; push unused error code

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

    mov rdi, [running]
    mov [rdi + 7 * 8], rsp ; save kernel rsp to running rsp
    
    call context_switch

    mov rdi, [running]
    mov rsp, [rdi + 7 * 8] ; now kernel rsp is the next task's rsp

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

    pop rax   ; pop unused error code

    iretq     ; restore rip, cs, rflags, rsp, ss

GLOBAL timer_handler
