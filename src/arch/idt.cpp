#include <arch/idt.h>
#include <task.h>

alignas(0x1000) IDT idt;

extern "C" void timer_handler(interrupt_frame* frame);

void IDT::init()
{
    kprintf(INFO "Initializing IDT...\n");

    pic::remap();
    pic::set_mask(0xffff);

    for (int i = 0; i < 256; i++)
        set(i, default_handler);

    set(13, gp_fault_handler);
    set(14, page_fault_handler);
    set(32, timer_handler);
    set(33, keyboard_handler);

    IDTDescriptor desc;
    desc.size = sizeof(IDT) - 1;
    desc.offset = (uint64_t)this;

    asm volatile("lidt %0" : : "m"(desc));
}

void IDT::set(uint8_t index, void (*isr)(interrupt_frame*))
{
    uint64_t offset = (uint64_t)isr;

    entries[index].offset_low = offset & 0xffff;
    entries[index].offset_mid = (offset >> 16) & 0xffff;
    entries[index].offset_high = offset >> 32;

    entries[index].ist = 0;             // no interrupt stack table
    entries[index].selector = KERNEL_CS;
    entries[index].type_attr = 0x8e;    // present, ring 0, interrupt gate
}

void default_handler(interrupt_frame* frame)
{
    panic("Unhandled interrupt");
}

void gp_fault_handler(interrupt_frame* frame)
{
    panic("General Protection Fault");
}

void page_fault_handler(interrupt_frame* frame)
{
    panic("Page Fault");
}

// void timer_handler(interrupt_frame* frame)
// {
//     kprintf("\e[92mtick\e[m\n");
//     pic::send_eoi(0);
// }

void keyboard_handler(interrupt_frame* frame)
{
    uint8_t scancode = inb(0x60);
    kprintf("scancode: \e[32m%hhx\e[m\n", scancode);
    pic::send_eoi(1);
}

extern "C" void context_switch()
{
    // kprintf("context switch\n");

    running = running->next;

    pic::send_eoi(0);
}