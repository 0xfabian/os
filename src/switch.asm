[bits 64]

extern running

first_switch:
    
    mov rdi, [running]
    mov rsp, [rdi + 7 * 8] ; moved to running stack

    mov rax, [rdi + 16 * 8] ; push rip
    push rax

    mov rax, [rdi + 17 * 8] ; push rflags
    push rax

    mov rax, [rdi + 0 * 8]
    mov rbx, [rdi + 1 * 8]
    mov rcx, [rdi + 2 * 8]
    mov rdx, [rdi + 3 * 8]
    mov rsi, [rdi + 4 * 8]

    mov rbp, [rdi + 6 * 8]

    mov r8, [rdi + 8 * 8]
    mov r9, [rdi + 9 * 8]
    mov r10, [rdi + 10 * 8]
    mov r11, [rdi + 11 * 8]
    mov r12, [rdi + 12 * 8]
    mov r13, [rdi + 13 * 8]
    mov r14, [rdi + 14 * 8]
    mov r15, [rdi + 15 * 8]

    mov rdi, [rdi + 5 * 8] ; finally restore rdi

    popfq ; restore rflags
    ret ; restore rip and also the stack is restored

GLOBAL first_switch

extern context_switch

; timer_handler:
;     iretq
;     ; save cpu state to running
;     push rax
;     push rcx
;     push rdx
;     push rsi
;     push rdi
;     push r8
;     push r9
;     push r10
;     push r11
;     cld
;     call context_switch
;     ; restore cpu state from running
;     pop r11
;     pop r10
;     pop r9
;     pop r8
;     pop rdi
;     pop rsi
;     pop rdx
;     pop rcx
;     pop rax
;     iretq

; timer_handler:

;     push rdi ; push rdi so that we can hold running
;     mov rdi, [running]

;     mov [rdi + 0 * 8], rax ; save rax

;     pop rax ; popped rdi
;     mov [rdi + 5 * 8], rax ; save rdi

;     pop rax ; popped rip from interrupt frame now
;     mov [rdi + 16 * 8], rax ; save rip

;     pop rax ; popped cs, don't care for now

;     pop rax ; popped rflags
;     mov [rdi + 17 * 8], rax ; save rflags

;     pop rax ; popped rsp
;     mov [rdi + 7 * 8], rax ; save rsp

;     pop rax ; popped ss, don't care for now

;     mov [rdi + 1 * 8], rbx
;     mov [rdi + 2 * 8], rcx
;     mov [rdi + 3 * 8], rdx
;     mov [rdi + 4 * 8], rsi

;     mov [rdi + 6 * 8], rbp

;     mov [rdi + 8 * 8], r8
;     mov [rdi + 9 * 8], r9
;     mov [rdi + 10 * 8], r10
;     mov [rdi + 11 * 8], r11
;     mov [rdi + 12 * 8], r12
;     mov [rdi + 13 * 8], r13
;     mov [rdi + 14 * 8], r14
;     mov [rdi + 15 * 8], r15

;     cld
;     call context_switch

;     mov rdi, [running]
;     mov rsp, [rdi + 7 * 8] ; moved to running stack

;     mov rax, [rdi + 16 * 8] ; push rip
;     push rax

;     mov rax, [rdi + 17 * 8] ; push rflags
;     push rax

;     mov rax, [rdi + 0 * 8]
;     mov rbx, [rdi + 1 * 8]
;     mov rcx, [rdi + 2 * 8]
;     mov rdx, [rdi + 3 * 8]
;     mov rsi, [rdi + 4 * 8]

;     mov rbp, [rdi + 6 * 8]

;     mov r8, [rdi + 8 * 8]
;     mov r9, [rdi + 9 * 8]
;     mov r10, [rdi + 10 * 8]
;     mov r11, [rdi + 11 * 8]
;     mov r12, [rdi + 12 * 8]
;     mov r13, [rdi + 13 * 8]
;     mov r14, [rdi + 14 * 8]
;     mov r15, [rdi + 15 * 8]

;     mov rdi, [rdi + 5 * 8] ; finally restore rdi

;     popfq ; restore rflags
;     ret ; restore rip and also the stack is restored

; GLOBAL timer_handler

timer_handler:

    push rax

    push rdi
    push rsi
    push rdx
    push rcx
    push rax
    push r8
    push r9
    push r10
    push r11

    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov rdi, [running]
    mov [rdi + 7 * 8], rsp ; moved to running stack
    
    call context_switch

    mov rdi, [running]
    mov rsp, [rdi + 7 * 8] ; moved to running stack

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    pop r11
    pop r10
    pop r9
    pop r8
    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi

    pop rax

    iretq

GLOBAL timer_handler
