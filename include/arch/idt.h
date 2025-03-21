#pragma once

#include <types.h>
#include <print.h>
#include <arch/pic.h>

struct interrupt_frame
{
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};

struct IDTDescriptor
{
    u16 size;
    u64 offset;
}
__attribute__((packed));

struct IDTEntry
{
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;
}
__attribute__((packed));

struct IDT
{
    IDTEntry entries[256];

    void init();
    void set(u8 index, void* isr);
    void set(u8 index, void (*isr)(interrupt_frame*));
    void set(u8 index, void (*isr)(interrupt_frame*, u64));
}
__attribute__((packed));

extern IDT idt;

__attribute__((interrupt)) void default_handler(interrupt_frame* frame);
__attribute__((interrupt)) void opcode_fault_handler(interrupt_frame* frame);
__attribute__((interrupt)) void gp_fault_handler(interrupt_frame* frame, u64 error_code);
__attribute__((interrupt)) void page_fault_handler(interrupt_frame* frame, u64 error_code);
__attribute__((interrupt)) void keyboard_handler(interrupt_frame* frame);