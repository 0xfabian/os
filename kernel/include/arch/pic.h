#pragma once

#include <types.h>
#include <arch/cpu.h>

#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xa0
#define PIC2_DATA       0xa1
#define PIC_EOI	        0x20

#define ICW1_INIT       0x10
#define ICW1_ICW4       0x01
#define ICW4_8086       0x01

namespace pic
{
    void remap(u8 offset1 = 0x20, u8 offset2 = 0x28);
    void disable();

    void set_mask(u16 mask);
    u16 get_mask();

    void mask_irq(u8 irq);
    void unmask_irq(u8 irq);
    bool get_irq_mask(u8 irq);

    void send_eoi(u8 irq);
}
