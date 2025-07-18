#include <arch/idt.h>
#include <arch/gdt.h>
#include <task.h>

alignas(0x1000) IDT idt;

// doesn't actually take an interrupt_frame as argument
// it's just a hack so it compiles when setting the handler
extern "C" void timer_handler_asm(interrupt_frame*);
extern "C" void breakpoint_handler_asm(interrupt_frame*);

void IDT::init()
{
    kprintf(INFO "Initializing IDT...\n");

    pic::remap();
    pic::set_mask(0xffff);

    for (int i = 0; i < 256; i++)
        set(i, default_handler);

    set(3, breakpoint_handler_asm, 3);
    set(6, opcode_fault_handler);
    set(13, gp_fault_handler);
    set(14, page_fault_handler);
    set(32, timer_handler_asm);
    set(33, keyboard_handler);

    IDTDescriptor desc;
    desc.size = sizeof(IDT) - 1;
    desc.offset = (u64)this;

    asm volatile("lidt %0" : : "m"(desc));
}

void IDT::set(u8 index, void* isr, u8 dpl)
{
    u64 offset = (u64)isr;

    entries[index].offset_low = offset & 0xffff;
    entries[index].offset_mid = (offset >> 16) & 0xffff;
    entries[index].offset_high = offset >> 32;

    entries[index].ist = 0;                                     // no interrupt stack table
    entries[index].selector = KERNEL_CS;
    entries[index].type_attr = 0b10001110 | ((dpl & 3) << 5);   // present, dpl, interrupt gate
}

void IDT::set(u8 index, void (*isr)(interrupt_frame*), u8 dpl)
{
    set(index, (void*)isr, dpl);
}

void IDT::set(u8 index, void (*isr)(interrupt_frame*, u64), u8 dpl)
{
    set(index, (void*)isr, dpl);
}

void default_handler(interrupt_frame* frame)
{
    if (!running)
        panic("Unhandled interrupt");

    kprintf("\a\e[37mKilled task %lu due to unhandled interrupt\e[m\n", running->tid);
    running->exit(-1);
}

__attribute__((interrupt)) void opcode_fault_handler(interrupt_frame* frame)
{
    if (!running)
        panic("Invalid opcode fault");

    kprintf("\a\e[37mKilled task %lu due to \e[91minvalid opcode\e[37m at address %p\e[m\n", running->tid, frame->rip);
    running->exit(-1);
}

void gp_fault_handler(interrupt_frame* frame, u64 error_code)
{
    if (!running)
        panic("General protection fault");

    kprintf("\a\e[37mKilled task %lu due to \e[91mgeneral protection fault\e[m\n", running->tid);
    running->exit(-1);
}

void page_fault_handler(interrupt_frame* frame, u64 error_code)
{
    if (!running)
        panic("Page fault");

    kprintf("\a\e[37mKilled task %lu due to \e[91mpage fault\e[37m at address %p\e[m\n", running->tid, read_cr2());
    running->exit(-1);
}

bool kbd_escape = false;
void keyboard_handler(interrupt_frame* frame)
{
    u8 key = inb(0x60);

    if (kbd_escape)
    {
        kbd_escape = false;
        key_queue.push(0xe0);
        key_queue.push(key);
    }
    else if (key == 0xe0)
        kbd_escape = true;
    else
        key_queue.push(key);

    // key_queue.push(key);

    // if (key == 0xe0)
    // {
    //     key = inb(0x60);
    //     key_queue.push(key);
    // }

    pic::send_eoi(1);
}

extern "C" void timer_handler()
{
    fbterm.tick();

    pic::send_eoi(0);

    schedule();
}

extern "C" void breakpoint_handler()
{
    kprintf("Breakpoint hit!\n\n");

    CPU* cpu = (CPU*)running->krsp;

    // rip will always be after the breakpoint, so we subtract 1
    kprintf("rip:    %a    cs:     %a\n", cpu->rip - 1, cpu->cs);
    kprintf("rflags: %a    ss:     %a\n", cpu->rflags, cpu->ss);
    kprintf("\n");
    kprintf("rax:    %a    r8:     %a\n", cpu->rax, cpu->r8);
    kprintf("rbx:    %a    r9:     %a\n", cpu->rbx, cpu->r9);
    kprintf("rcx:    %a    r10:    %a\n", cpu->rcx, cpu->r10);
    kprintf("rdx:    %a    r11:    %a\n", cpu->rdx, cpu->r11);
    kprintf("rsi:    %a    r12:    %a\n", cpu->rsi, cpu->r12);
    kprintf("rdi:    %a    r13:    %a\n", cpu->rdi, cpu->r13);
    kprintf("rbp:    %a    r14:    %a\n", cpu->rbp, cpu->r14);
    kprintf("rsp:    %a    r15:    %a\n", cpu->rsp, cpu->r15);

    pause();
}
