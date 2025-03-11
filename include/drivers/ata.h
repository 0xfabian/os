#pragma once

#include <mem/heap.h>
#include <arch/cpu.h>

struct ATA
{
    u16 data;
    u16 error;
    u16 sector_count;
    u16 lba_low;
    u16 lba_mid;
    u16 lba_high;
    u16 device;
    u16 command;
    u16 control;
    u16 byte_per_sector;
    bool master;

    static ATA* from(u16 port_base, bool master);

    bool identify();
    bool read(u32 lba, u8* buf, u32 count);
    bool write(u32 lba, u8* buf, u32 count);
    bool flush();
};