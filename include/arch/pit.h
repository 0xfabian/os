#pragma once

#include <types.h>
#include <arch/cpu.h>

#define PIT_CHANNEL_0_PORT  0x40
#define PIT_COMMAND_PORT    0x43

namespace pit
{
    void init(u32 freq);
}