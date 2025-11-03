#include <arch/pit.h>

void pit::init(u32 freq)
{
    u32 input_frequency = 1193182;

    if (freq < 18)
        freq = 18;

    if (freq > input_frequency)
        freq = input_frequency - 1;

    u16 counter = input_frequency / freq;

    if (counter == 0)
        counter++;

    // bits & usage:

    // 6 and 7: select channel
    // 0 0 = channel 0
    // 0 1 = channel 1
    // 1 0 = channel 2
    // 1 1 = read-back command (8254 only)

    // 4 and 5: access mode
    // 0 0 = latch count value command
    // 0 1 = access mode lobyte only
    // 1 0 = access mode hibyte only
    // 1 1 = access mode lobyte/hibyte

    // 1 to 3: operating mode
    // 0 0 0 = mode 0 (interrupt on terminal count)
    // 0 0 1 = mode 1 (hardware re-triggerable one-shot)
    // 0 1 0 = mode 2 (rate generator)
    // 0 1 1 = mode 3 (square wave generator)
    // 1 0 0 = mode 4 (software triggered strobe)
    // 1 0 1 = mode 5 (hardware triggered strobe)
    // 1 1 0 = mode 2 (rate generator, same as 010b)
    // 1 1 1 = mode 3 (square wave generator, same as 011b)

    // 0: bcd/binary mode
    // 0 = 16-bit binary
    // 1 = four-digit bcd

    outb(PIT_COMMAND_PORT, 0b00110100);
    outb(PIT_CHANNEL_0_PORT, counter & 0xff);
    outb(PIT_CHANNEL_0_PORT, counter >> 8);
}
