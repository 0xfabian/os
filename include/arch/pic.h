#pragma once

#include <cstdint>
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
    void remap(uint8_t offset1 = 0x20, uint8_t offset2 = 0x28);
    void set_mask(uint16_t mask);
    void send_eoi(uint8_t irq);
    void disable();
}