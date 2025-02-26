#pragma once

#include <cstdint>
#include <print.h>
#include <arch/pic.h>

struct interrupt_frame
{
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

struct IDTDescriptor
{
    uint16_t size;
    uint64_t offset;
}
__attribute__((packed));

struct IDTEntry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
}
__attribute__((packed));

struct IDT
{
    IDTEntry entries[256];

    void init();
    void set(uint8_t index, void* isr);
    void set(uint8_t index, void (*isr)(interrupt_frame*));
    void set(uint8_t index, void (*isr)(interrupt_frame*, uint64_t));
}
__attribute__((packed));

extern IDT idt;

__attribute__((interrupt)) void default_handler(interrupt_frame* frame);
__attribute__((interrupt)) void gp_fault_handler(interrupt_frame* frame, uint64_t error_code);
__attribute__((interrupt)) void page_fault_handler(interrupt_frame* frame, uint64_t error_code);
__attribute__((interrupt)) void keyboard_handler(interrupt_frame* frame);