[bits 64]

load_gdt_desc:   
    lgdt [rdi]
    mov ax, 0x10    ; kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rdi         ; pop the return address
    mov rax, 0x08   ; kernel code segment
    push rax        ; push the new cs
    push rdi        ; push the return address
    retfq           ; sets cs and returns

global load_gdt_desc