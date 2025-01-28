#include <arch/pic.h>

void pic::remap(uint8_t offset1, uint8_t offset2)
{
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

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

void pic::set_mask(uint16_t mask)
{
    uint8_t mask1 = mask & 0xff;
    uint8_t mask2 = mask >> 8;

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

uint16_t pic::get_mask()
{
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    return mask1 | (mask2 << 8);
}

void pic::set_irq(uint8_t irq, bool masked)
{
    uint16_t mask = get_mask();

    if (masked)
        mask |= 1 << irq;
    else
        mask &= ~(1 << irq);

    set_mask(mask);
}

bool pic::get_irq(uint8_t irq)
{
    return get_mask() & (1 << irq);
}

void pic::send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}