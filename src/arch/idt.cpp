#include <arch/idt.h>
#include <arch/gdt.h>
#include <task.h>

alignas(0x1000) IDT idt;

extern "C" void timer_handler_asm(interrupt_frame* frame);

void IDT::init()
{
    kprintf(INFO "Initializing IDT...\n");

    pic::remap();
    pic::set_mask(0xffff);

    for (int i = 0; i < 256; i++)
        set(i, default_handler);

    set(13, gp_fault_handler);
    set(14, page_fault_handler);
    set(32, timer_handler_asm);
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
        kprintf(PANIC "Unhandled interrupt in task %lu\n", running->tid);
        idle();
    }
    else
        panic("Unhandled interrupt");
}

void gp_fault_handler(interrupt_frame* frame, u64 error_code)
{
    if (running)
    {
        kprintf(PANIC "General Protection Fault in task %lu (%lx)\n", running->tid, error_code);
        idle();
    }
    else
        panic("General Protection Fault");
}

void page_fault_handler(interrupt_frame* frame, u64 error_code)
{
    if (running)
    {
        kprintf(PANIC "Page Fault in task %lu (%lx)\n", running->tid, error_code);
        idle();
    }
    else
        panic("Page Fault");
}

void keyboard_handler(interrupt_frame* frame)
{
    // // e0 53 delete     \e[3~
    // // e0 47 home       \e[H
    // // e0 4f end        \e[F
    // // e0 48 up         \e[A
    // // e0 50 down       \e[B
    // // e0 4d right      \e[C
    // // e0 4b left       \e[D
    // // 1;2 shift modifier

    // u8 key = inb(0x60);
    // bool extended = false;

    // if (key == 0xe0)
    // {
    //     extended = true;
    //     key = inb(0x60);
    // }

    // if (extended && (key & 0x80) == 0)
    // {
    //     if (fbterm.echo)
    //     {
    //         fbterm.receive_char('^');
    //         fbterm.receive_char('[');
    //     }
    //     else
    //         fbterm.receive_char('\e');

    //     fbterm.receive_char('[');

    //     if (key == 0x53)
    //     {
    //         fbterm.receive_char('3');
    //         fbterm.receive_char('~');
    //     }
    //     else
    //     {
    //         if (kbd_state & KBD_LSHIFT || kbd_state & KBD_RSHIFT)
    //         {
    //             fbterm.receive_char('1');
    //             fbterm.receive_char(';');
    //             fbterm.receive_char('2');
    //         }

    //         if (key == 0x47)
    //             fbterm.receive_char('H');
    //         else if (key == 0x4f)
    //             fbterm.receive_char('F');
    //         else if (key == 0x48)
    //             fbterm.receive_char('A');
    //         else if (key == 0x50)
    //             fbterm.receive_char('B');
    //         else if (key == 0x4d)
    //             fbterm.receive_char('C');
    //         else if (key == 0x4b)
    //             fbterm.receive_char('D');
    //     }
    // }
    // else
    // {
    //     char ch = translate_key(key);

    //     if (ch == 'd' && kbd_state & KBD_CTRL)
    //         fbterm.handle_requests();
    //     else if (ch == 'u' && kbd_state & KBD_CTRL)
    //         fbterm.clear_input();
    //     else if (ch)
    //         fbterm.receive_char(ch);
    // }

    u8 key = inb(0x60);
    key_queue.push(key);

    if (key == 0xe0)
    {
        key = inb(0x60);
        key_queue.push(key);
    }

    pic::send_eoi(1);
}

extern "C" void timer_handler()
{
    fbterm.tick();

    pic::send_eoi(0);

    schedule();
}