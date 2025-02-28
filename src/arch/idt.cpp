#include <arch/idt.h>
#include <arch/gdt.h>
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
    desc.offset = (u64)this;

    asm volatile("lidt %0" : : "m"(desc));
}

void IDT::set(u8 index, void* isr)
{
    u64 offset = (u64)isr;

    entries[index].offset_low = offset & 0xffff;
    entries[index].offset_mid = (offset >> 16) & 0xffff;
    entries[index].offset_high = offset >> 32;

    entries[index].ist = 0;             // no interrupt stack table
    entries[index].selector = KERNEL_CS;
    entries[index].type_attr = 0x8e;    // present, ring 0, interrupt gate
}

void IDT::set(u8 index, void (*isr)(interrupt_frame*))
{
    set(index, (void*)isr);
}

void IDT::set(u8 index, void (*isr)(interrupt_frame*, u64))
{
    set(index, (void*)isr);
}

void default_handler(interrupt_frame* frame)
{
    if (running)
    {
        ikprintf(PANIC "Unhandled interrupt in task %lu\n", running->tid);
        idle();
    }
    else
        panic("Unhandled interrupt");
}

void gp_fault_handler(interrupt_frame* frame, u64 error_code)
{
    if (running)
    {
        ikprintf(PANIC "General Protection Fault in task %lu (%lx)\n", running->tid, error_code);
        idle();
    }
    else
        panic("General Protection Fault");
}

void page_fault_handler(interrupt_frame* frame, u64 error_code)
{
    if (running)
    {
        ikprintf(PANIC "Page Fault in task %lu (%lx)\n", running->tid, error_code);
        idle();
    }
    else
        panic("Page Fault");
}

void keyboard_handler(interrupt_frame* frame)
{
    u8 scancode = inb(0x60);
    kprintf("scancode: \e[32m%hhx\e[m\n", scancode);
    pic::send_eoi(1);
}

int ncs = 3;

extern "C" void context_switch()
{
    // if (ncs-- <= 0)
    //     idle();

    // ikprintf("context_switch(): %lu -> %lu\n", running->tid, running->next->tid);

    // CPU* cpu = (CPU*)running->krsp;

    // ikprintf("from cs: %lx    ss: %lx     rsp: %lx    stack: %lx    kstack: %lx\n", cpu->cs, cpu->ss, cpu->rsp, running->mm.user_stack, running->mm.kernel_stack);
    // u64 rsp;
    // asm volatile ("mov %%rsp, %0" : "=r"(rsp));
    // ikprintf("current rsp: %lx\n", rsp);

    running = running->next;

    // CPU* cpu = (CPU*)running->krsp;

    // ikprintf("to cs: %lx    ss: %lx\n", cpu->cs, cpu->ss);

    if (running->mm.user_stack)
        tss.rsp0 = (u64)running->mm.kernel_stack + 4096;

    pic::send_eoi(0);

    // if (running->tid == 2)
    //     pic::set_irq(0, true);
}