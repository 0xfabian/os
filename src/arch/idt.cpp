#include <arch/idt.h>

alignas(0x1000) IDT idt;

void IDT::init()
{
    kprintf(INFO "Initializing IDT...\n");

    pic::remap();
    pic::set_mask(0xffff);

    for (int i = 0; i < 256; i++)
        set(i, default_handler);

    set(13, gp_fault_handler);
    set(14, page_fault_handler);
    set(33, keyboard_handler);

    IDTDescriptor desc;
    desc.size = sizeof(IDT) - 1;
    desc.offset = (uint64_t)this;

    asm volatile("lidt %0" : : "m"(desc));
    asm volatile("sti");
}

void IDT::set(uint8_t index, void (*isr)(interrupt_frame*))
{
    uint64_t isr_addr = (uint64_t)isr;

    entries[index].offset_low = isr_addr & 0xffff;
    entries[index].offset_mid = (isr_addr >> 16) & 0xffff;
    entries[index].offset_high = isr_addr >> 32;

    entries[index].ist = 0;             // no interrupt stack table
    entries[index].selector = 0x08;     // kernel_cs
    entries[index].type_attr = 0x8e;    // present, ring 0, interrupt gate
}

__attribute__((interrupt)) void default_handler(interrupt_frame* frame)
{
    panic("Unhandled interrupt");
}

__attribute__((interrupt)) void gp_fault_handler(interrupt_frame* frame)
{
    panic("General Protection Fault");
}

__attribute__((interrupt)) void page_fault_handler(interrupt_frame* frame)
{
    panic("Page Fault");
}

__attribute__((interrupt)) void keyboard_handler(interrupt_frame* frame)
{
    uint8_t scancode = inb(0x60);
    kprintf("scancode: %hhx\n", scancode);
    pic::send_eoi(1);
}