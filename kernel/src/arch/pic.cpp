#include <arch/pic.h>

void pic::remap(u8 offset1, u8 offset2)
{
    u8 mask1 = inb(PIC1_DATA);
    u8 mask2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic::disable()
{
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void pic::set_mask(u16 mask)
{
    u8 mask1 = mask & 0xff;
    u8 mask2 = mask >> 8;

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

u16 pic::get_mask()
{
    u8 mask1 = inb(PIC1_DATA);
    u8 mask2 = inb(PIC2_DATA);

    return mask1 | (mask2 << 8);
}

void pic::mask_irq(u8 irq)
{
    u16 mask = get_mask();

    mask |= 1 << irq;

    set_mask(mask);
}

void pic::unmask_irq(u8 irq)
{
    u16 mask = get_mask();

    mask &= ~(1 << irq);

    set_mask(mask);
}

bool pic::get_irq_mask(u8 irq)
{
    return get_mask() & (1 << irq);
}

void pic::send_eoi(u8 irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}
